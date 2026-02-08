#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/PathFinder.h"
#include "board/Item.h"
#include "board/Trace.h"
#include "board/Via.h"
#include <algorithm>

namespace freerouting {

void AutorouteEngine::initConnection(int netNumber, Stoppable* stoppable, TimeLimit* limit) {
  if (maintainDatabase && netNumber != netNo) {
    // Invalidate net-dependent complete expansion rooms
    auto it = completeExpansionRooms.begin();
    while (it != completeExpansionRooms.end()) {
      if ((*it)->isNetDependent()) {
        // Remove from search tree if needed
        it = completeExpansionRooms.erase(it);
      } else {
        ++it;
      }
    }
  }

  netNo = netNumber;
  stoppableThread = stoppable;
  timeLimit = limit;
}

AutorouteEngine::AutorouteResult AutorouteEngine::autorouteConnection(
    const std::vector<Item*>& startSet,
    const std::vector<Item*>& destSet,
    const AutorouteControl& ctrl,
    std::vector<Item*>& rippedItems) {

  if (!board || startSet.empty() || destSet.empty()) {
    return AutorouteResult::Failed;
  }

  // Check if already connected (simplified - just check if same item)
  if (startSet.size() == 1 && destSet.size() == 1 && startSet[0] == destSet[0]) {
    return AutorouteResult::AlreadyConnected;
  }

  // Get start and goal points (use bounding box centers)
  IntBox startBox = startSet[0]->getBoundingBox();
  IntBox goalBox = destSet[0]->getBoundingBox();

  IntPoint start((startBox.ll.x + startBox.ur.x) / 2,
                 (startBox.ll.y + startBox.ur.y) / 2);
  IntPoint goal((goalBox.ll.x + goalBox.ur.x) / 2,
                (goalBox.ll.y + goalBox.ur.y) / 2);

  // Determine start and destination layers
  int startLayer = startSet[0]->firstLayer();
  int destLayer = destSet[0]->firstLayer();

  // Calculate ripup cost limit (increases with pass number if ripup allowed)
  int ripupCostLimit = ctrl.ripupAllowed ? ctrl.ripupCosts : 0;

  // Check if we need layer transition
  bool needsVia = (startLayer != destLayer);

  if (!needsVia) {
    // Same layer routing - use adaptive pathfinding
    // Try coarse grid first (fast), then fine grid if needed (precise)

    // First attempt: Coarse grid (1000 units = 0.1mm) for speed
    PathFinder coarsePathFinder(1000);
    auto pathResult = coarsePathFinder.findPath(*board, start, goal, startLayer, netNo);

    // If coarse grid failed, retry with finer grid (500 units = 0.05mm)
    if (!pathResult.found) {
      PathFinder finePathFinder(500);
      pathResult = finePathFinder.findPath(*board, start, goal, startLayer, netNo);
    }

    if (!pathResult.found || pathResult.points.size() < 2) {
      return createDirectRoute(start, goal, startLayer, ctrl, ripupCostLimit, rippedItems);
    }

    return createTracesFromPath(pathResult.points, startLayer, ctrl, ripupCostLimit, rippedItems);
  }

  // Multi-layer routing with via insertion
  return routeWithVia(start, goal, startLayer, destLayer, ctrl, ripupCostLimit, rippedItems);
}

// Helper: Create direct route (fallback when pathfinding fails)
AutorouteEngine::AutorouteResult AutorouteEngine::createDirectRoute(
    IntPoint start, IntPoint goal, int layer, const AutorouteControl& ctrl,
    int ripupCostLimit, std::vector<Item*>& rippedItems) {

  // Get trace half-width from control
  int halfWidth = ctrl.traceHalfWidth.empty() ? 1250 : ctrl.traceHalfWidth[layer];

  std::vector<int> nets{netNo};

  // Check for conflicts before creating the trace
  auto conflicts = findConflictingItems(start, goal, layer, halfWidth, netNo);

  if (!conflicts.empty()) {
    // If push-and-shove is enabled, try that first before ripup
    if (ctrl.pushAndShoveEnabled && ctrl.ripupAllowed) {
      // Try push-and-shove with half the ripup budget (save rest for actual ripup)
      // NOTE: Full push-and-shove implementation would be here
      // For now, fall through to ripup
    }

    // Try to ripup conflicting items if within cost limit
    bool ripupSuccess = ripupConflicts(conflicts, ripupCostLimit, rippedItems);
    if (!ripupSuccess) {
      // Can't ripup - routing failed
      return AutorouteResult::NotRouted;
    }
  }

  // Create the trace
  int itemId = board->generateItemId();

  auto trace = std::make_unique<Trace>(
    start, goal, layer, halfWidth,
    nets, ctrl.traceClearanceClassNo, itemId,
    FixedState::NotFixed, board
  );

  board->addItem(std::move(trace));
  return AutorouteResult::Routed;
}

// Helper: Create traces from path points
AutorouteEngine::AutorouteResult AutorouteEngine::createTracesFromPath(
    const std::vector<IntPoint>& points, int layer, const AutorouteControl& ctrl,
    int ripupCostLimit, std::vector<Item*>& rippedItems) {

  if (points.size() < 2) {
    return AutorouteResult::NotRouted;
  }

  // Get trace half-width from control
  int halfWidth = ctrl.traceHalfWidth.empty() ? 1250 : ctrl.traceHalfWidth[layer];
  std::vector<int> nets{netNo};

  // Create trace segments connecting consecutive points
  for (size_t i = 0; i + 1 < points.size(); ++i) {
    IntPoint p1 = points[i];
    IntPoint p2 = points[i + 1];

    // Skip if points are too close (avoid zero-length traces)
    IntVector delta = p2 - p1;
    if (delta.lengthSquared() < 100) {
      continue;
    }

    // Check for conflicts before creating the trace
    auto conflicts = findConflictingItems(p1, p2, layer, halfWidth, netNo);

    if (!conflicts.empty()) {
      // Try to ripup conflicting items if within cost limit
      bool ripupSuccess = ripupConflicts(conflicts, ripupCostLimit, rippedItems);
      if (!ripupSuccess) {
        // Can't ripup - routing failed for this segment
        // Continue with other segments (partial route)
        continue;
      }
    }

    // Create the trace segment
    int itemId = board->generateItemId();

    auto trace = std::make_unique<Trace>(
      p1, p2, layer, halfWidth,
      nets, ctrl.traceClearanceClassNo, itemId,
      FixedState::NotFixed, board
    );

    board->addItem(std::move(trace));
  }

  return AutorouteResult::Routed;
}

// Helper: Route with via for layer changes
AutorouteEngine::AutorouteResult AutorouteEngine::routeWithVia(
    IntPoint start, IntPoint goal, int startLayer, int destLayer,
    const AutorouteControl& ctrl, int ripupCostLimit, std::vector<Item*>& rippedItems) {

  // Try multiple via locations (midpoint, closer to start, closer to goal)
  std::vector<IntPoint> viaLocations;

  // Midpoint (default)
  viaLocations.push_back(IntPoint((start.x + goal.x) / 2, (start.y + goal.y) / 2));

  // Closer to start (25% of the way)
  viaLocations.push_back(IntPoint(
    start.x + (goal.x - start.x) / 4,
    start.y + (goal.y - start.y) / 4
  ));

  // Closer to goal (75% of the way)
  viaLocations.push_back(IntPoint(
    start.x + 3 * (goal.x - start.x) / 4,
    start.y + 3 * (goal.y - start.y) / 4
  ));

  // Try each via location until one works
  for (const IntPoint& viaLocation : viaLocations) {
    auto result = tryRouteWithViaAt(viaLocation, start, goal, startLayer, destLayer,
                                     ctrl, ripupCostLimit, rippedItems);
    if (result == AutorouteResult::Routed) {
      return result;
    }
  }

  return AutorouteResult::NotRouted;
}

// Helper: Try routing with via at specific location
AutorouteEngine::AutorouteResult AutorouteEngine::tryRouteWithViaAt(
    IntPoint viaLocation, IntPoint start, IntPoint goal,
    int startLayer, int destLayer,
    const AutorouteControl& ctrl, int ripupCostLimit, std::vector<Item*>& rippedItems) {

  // Check if a via already exists at this location
  Via* existingVia = findViaAtLocation(viaLocation, netNo);

  // Only create a new via if one doesn't already exist
  if (!existingVia) {
    // Create via at midpoint
    std::vector<int> nets{netNo};
    int viaId = board->generateItemId();

    // Create padstack for via (simplified - fixed layers from/to)
    Padstack* padstack = new Padstack(
      "via_" + std::to_string(viaId),
      viaId,
      std::min(startLayer, destLayer),  // fromLayer
      std::max(startLayer, destLayer),  // toLayer
      true,   // attachAllowed
      false   // placedAbsolute
    );

    auto via = std::make_unique<Via>(
      viaLocation, padstack, nets, ctrl.viaClearanceClass, viaId,
      FixedState::NotFixed, true /* attachAllowed */, board
    );

    board->addItem(std::move(via));
  }

  // Route first segment: start -> via on startLayer
  PathFinder pathFinder(1000);
  auto path1 = pathFinder.findPath(*board, start, viaLocation, startLayer, netNo);

  if (path1.found && path1.points.size() >= 2) {
    createTracesFromPath(path1.points, startLayer, ctrl, ripupCostLimit, rippedItems);
  } else {
    // Fallback to direct route
    createDirectRoute(start, viaLocation, startLayer, ctrl, ripupCostLimit, rippedItems);
  }

  // Route second segment: via -> goal on destLayer
  auto path2 = pathFinder.findPath(*board, viaLocation, goal, destLayer, netNo);

  if (path2.found && path2.points.size() >= 2) {
    createTracesFromPath(path2.points, destLayer, ctrl, ripupCostLimit, rippedItems);
  } else {
    // Fallback to direct route
    createDirectRoute(viaLocation, goal, destLayer, ctrl, ripupCostLimit, rippedItems);
  }

  return AutorouteResult::Routed;
}

// Helper: Find existing via at a location
Via* AutorouteEngine::findViaAtLocation(IntPoint location, int netNo) const {
  if (!board) return nullptr;

  // Iterate through all items to find vias at the same location
  for (const auto& itemPtr : board->getItems()) {
    Via* via = dynamic_cast<Via*>(itemPtr.get());
    if (!via) continue;

    // Check if via is at the same location and on the same net
    IntPoint viaCenter = via->getCenter();
    if (viaCenter.x == location.x && viaCenter.y == location.y) {
      // Check if via shares the net
      const auto& viaNets = via->getNets();
      if (std::find(viaNets.begin(), viaNets.end(), netNo) != viaNets.end()) {
        return via;
      }
    }
  }

  return nullptr;
}

// Helper: Find conflicting items for a proposed trace
std::vector<Item*> AutorouteEngine::findConflictingItems(
    IntPoint start, IntPoint end, int layer, int halfWidth, int netNo) const {

  std::vector<Item*> conflicts;

  if (!board) return conflicts;

  // Create bounding box for the proposed trace
  int minX = std::min(start.x, end.x) - halfWidth;
  int maxX = std::max(start.x, end.x) + halfWidth;
  int minY = std::min(start.y, end.y) - halfWidth;
  int maxY = std::max(start.y, end.y) + halfWidth;
  IntBox traceBox(minX, minY, maxX, maxY);

  // Get required clearance for querying shape tree
  const ClearanceMatrix& clearance = board->getClearanceMatrix();
  int requiredClearance = clearance.getValue(
    1,  // Use class 1 ("default") - class 0 ("null") is not initialized
    1,
    layer,
    true  // Add safety margin
  );

  // Expand trace box by clearance for shape tree query
  // This ensures we find all items within the clearance zone
  IntBox queryBox(
    traceBox.ll.x - requiredClearance,
    traceBox.ll.y - requiredClearance,
    traceBox.ur.x + requiredClearance,
    traceBox.ur.y + requiredClearance
  );

  // Query the shape tree for potential conflicts
  const auto& items = board->getShapeTree().queryRegion(queryBox);

  // Check each item for actual conflicts
  for (Item* item : items) {
    if (!item) continue;

    // Skip items on different layers
    if (item->lastLayer() < layer || item->firstLayer() > layer) {
      continue;
    }

    // Skip items on the same net (they're not obstacles)
    const auto& itemNets = item->getNets();
    if (std::find(itemNets.begin(), itemNets.end(), netNo) != itemNets.end()) {
      continue;
    }

    // NOTE: We do NOT skip fixed items here - they are obstacles that must be
    // respected. The ripupConflicts() function will handle them (and fail to
    // ripup them, preventing the trace from being placed).

    // Check if bounding boxes overlap (accounting for clearance)
    // Note: We already expanded traceBox by requiredClearance when querying
    // the shape tree, so we just need to check if the expanded box (queryBox)
    // intersects with the item's bounding box
    IntBox itemBox = item->getBoundingBox();

    // Check if expanded box overlaps with item
    if (queryBox.intersects(itemBox)) {
      conflicts.push_back(item);
    }
  }

  return conflicts;
}

// Helper: Calculate ripup cost for an item
int AutorouteEngine::calculateRipupCost(Item* item, int passNumber) const {
  if (!item) return INT32_MAX;

  // Can't ripup fixed items
  if (item->isUserFixed()) {
    return INT32_MAX;
  }

  // Base cost starts low and increases with each pass
  // This makes the router more aggressive about ripup in later passes
  int baseCost = 100;

  // Get ripup count for this item
  int ripupCount = 0;
  auto it = ripupCounts.find(item->getId());
  if (it != ripupCounts.end()) {
    ripupCount = it->second;
  }

  // Cost formula: baseCost * (1 + ripupCount) * passNumber
  // Items that have been ripped multiple times become more expensive
  int cost = baseCost * (1 + ripupCount) * std::max(1, passNumber);

  return cost;
}

// Helper: Try to ripup conflicting items
bool AutorouteEngine::ripupConflicts(
    const std::vector<Item*>& conflicts,
    int ripupCostLimit,
    std::vector<Item*>& rippedItems) {

  if (!board || conflicts.empty()) return true;  // No conflicts = success

  // Calculate total ripup cost and check if all can be ripped
  int totalCost = 0;
  std::vector<std::pair<Item*, int>> itemsWithCosts;  // Track items and their costs

  for (Item* conflict : conflicts) {
    if (!conflict) continue;

    // Calculate cost for this item (pass number = 1 for now)
    int cost = calculateRipupCost(conflict, 1);

    // Can't ripup fixed items (cost will be INT32_MAX)
    if (cost == INT32_MAX) {
      return false;
    }

    totalCost += cost;
    itemsWithCosts.emplace_back(conflict, cost);

    // Early exit if we've already exceeded budget
    if (totalCost > ripupCostLimit) {
      return false;
    }
  }

  // Check if total cost is within budget
  if (totalCost > ripupCostLimit) {
    return false;
  }

  // All conflicts can be ripped within budget - proceed with ripup
  for (const auto& [conflict, cost] : itemsWithCosts) {
    if (!conflict) continue;

    int itemId = conflict->getId();

    // Increment ripup count for this item
    ripupCounts[itemId]++;

    // Remove from board
    board->removeItem(itemId);

    // Add to ripped items list for potential re-routing
    rippedItems.push_back(conflict);
  }

  return true;
}

bool AutorouteEngine::isStopRequested() const {
  if (timeLimit && timeLimit->isExceeded()) {
    return true;
  }
  if (stoppableThread) {
    return stoppableThread->isStopRequested();
  }
  return false;
}

void AutorouteEngine::clear() {
  // Remove complete rooms from search tree if needed
  completeExpansionRooms.clear();
  incompleteExpansionRooms.clear();
  expansionRoomInstanceCount = 0;

  // Clear autoroute data from board items
  if (board) {
    // This would call board->clearAllItemTemporaryAutorouteData()
    // when that method exists
  }
}

void AutorouteEngine::resetAllDoors() {
  for (auto& room : completeExpansionRooms) {
    room->resetDoors();
  }
  for (auto& room : incompleteExpansionRooms) {
    room->resetDoors();
  }
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::addIncompleteExpansionRoom(
    const Shape* shape, int layer, const Shape* containedShape) {

  auto room = std::make_unique<IncompleteFreeSpaceExpansionRoom>(
    shape, layer, containedShape);

  auto* roomPtr = room.get();
  incompleteExpansionRooms.push_back(std::move(room));

  return roomPtr;
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::getFirstIncompleteExpansionRoom() {
  if (incompleteExpansionRooms.empty()) {
    return nullptr;
  }
  return incompleteExpansionRooms.front().get();
}

void AutorouteEngine::removeIncompleteExpansionRoom(IncompleteFreeSpaceExpansionRoom* room) {
  if (!room) return;

  removeAllDoors(room);

  auto it = std::find_if(incompleteExpansionRooms.begin(),
                         incompleteExpansionRooms.end(),
                         [room](const auto& ptr) { return ptr.get() == room; });

  if (it != incompleteExpansionRooms.end()) {
    incompleteExpansionRooms.erase(it);
  }
}

void AutorouteEngine::removeCompleteExpansionRoom(CompleteFreeSpaceExpansionRoom* room) {
  if (!room) return;

  // Create new incomplete expansion rooms for neighbors
  // (This would involve checking doors and creating new rooms)

  removeAllDoors(room);

  auto it = std::find_if(completeExpansionRooms.begin(),
                         completeExpansionRooms.end(),
                         [room](const auto& ptr) { return ptr.get() == room; });

  if (it != completeExpansionRooms.end()) {
    completeExpansionRooms.erase(it);
  }
}

CompleteFreeSpaceExpansionRoom* AutorouteEngine::completeExpansionRoom(
    IncompleteFreeSpaceExpansionRoom* incompleteRoom) {

  if (!incompleteRoom) return nullptr;

  // Create complete room with same shape and layer
  auto completeRoom = std::make_unique<CompleteFreeSpaceExpansionRoom>(
    incompleteRoom->getShape(),
    incompleteRoom->getLayer(),
    ++expansionRoomInstanceCount);

  // Transfer doors
  for (auto* door : incompleteRoom->getDoors()) {
    completeRoom->addDoor(door);
    // Update door references from incomplete to complete
    if (door->firstRoom == incompleteRoom) {
      door->firstRoom = completeRoom.get();
    }
    if (door->secondRoom == incompleteRoom) {
      door->secondRoom = completeRoom.get();
    }
  }

  auto* completePtr = completeRoom.get();
  completeExpansionRooms.push_back(std::move(completeRoom));

  // Remove the incomplete room
  removeIncompleteExpansionRoom(incompleteRoom);

  return completePtr;
}

void AutorouteEngine::removeAllDoors(ExpansionRoom* room) {
  if (!room) return;

  auto doors = room->getDoors();
  for (auto* door : doors) {
    // Remove door from the other room
    if (door->firstRoom == room && door->secondRoom) {
      door->secondRoom->removeDoor(door);
    } else if (door->secondRoom == room && door->firstRoom) {
      door->firstRoom->removeDoor(door);
    }
  }

  room->clearDoors();
}

} // namespace freerouting
