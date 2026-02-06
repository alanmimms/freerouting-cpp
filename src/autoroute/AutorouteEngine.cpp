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
    // Same layer routing - use simple pathfinding
    PathFinder pathFinder(1000);  // 1000 unit grid = 0.1mm
    auto pathResult = pathFinder.findPath(*board, start, goal, startLayer, netNo);

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

  // Calculate via location (midpoint between start and goal)
  IntPoint viaLocation(
    (start.x + goal.x) / 2,
    (start.y + goal.y) / 2
  );

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

  // Query the shape tree for potential conflicts
  const auto& items = board->getShapeTree().queryRegion(traceBox);

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

    // Skip fixed items (we can't ripup user-placed traces)
    if (item->isUserFixed()) {
      continue;
    }

    // Check if bounding boxes overlap (accounting for clearance)
    IntBox itemBox = item->getBoundingBox();

    // Get required clearance
    const ClearanceMatrix& clearance = board->getClearanceMatrix();
    int requiredClearance = clearance.getValue(
      0,  // Assume class 0 for now (should use actual clearance classes)
      0,
      layer,
      true  // Add safety margin
    );

    // Expand trace box by required clearance
    IntBox expandedBox(
      traceBox.ll.x - requiredClearance,
      traceBox.ll.y - requiredClearance,
      traceBox.ur.x + requiredClearance,
      traceBox.ur.y + requiredClearance
    );

    // Check if expanded box overlaps with item
    if (expandedBox.intersects(itemBox)) {
      conflicts.push_back(item);
    }
  }

  return conflicts;
}

// Helper: Calculate ripup cost for an item
int AutorouteEngine::calculateRipupCost(Item* item, int passNumber) const {
  if (!item) return INT32_MAX;

  // Base cost starts low and increases with each pass
  // This makes the router more aggressive about ripup in later passes
  int baseCost = 100;

  // Increase cost for each previous ripup of this item
  // (stored in autoroute info if we had that infrastructure)
  int ripupCount = 0;  // TODO: Track ripup count per item

  // Cost formula: baseCost * (1 + ripupCount) * passNumber
  int cost = baseCost * (1 + ripupCount) * std::max(1, passNumber);

  return cost;
}

// Helper: Try to ripup conflicting items
bool AutorouteEngine::ripupConflicts(
    const std::vector<Item*>& conflicts,
    int ripupCostLimit,
    std::vector<Item*>& rippedItems) {

  if (!board) return false;

  // Check if all conflicts can be ripped within cost limit
  for (Item* conflict : conflicts) {
    if (!conflict) continue;

    // Can't ripup fixed items
    if (conflict->isUserFixed()) {
      return false;
    }

    // Check ripup cost (for now, simplified - just check pass number)
    // In full implementation, would track ripup history per item
  }

  // Ripup all conflicts
  for (Item* conflict : conflicts) {
    if (!conflict) continue;

    // Remove from board
    board->removeItem(conflict->getId());

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
