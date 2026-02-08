#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "board/Item.h"
#include <cmath>
#include <algorithm>

namespace freerouting {

std::unique_ptr<MazeSearchAlgo> MazeSearchAlgo::getInstance(
    const std::vector<Item*>& startItems,
    const std::vector<Item*>& destItems,
    AutorouteEngine* engine,
    const AutorouteControl& ctrl) {

  auto algo = std::make_unique<MazeSearchAlgo>(engine, ctrl);

  if (!algo->init(startItems, destItems)) {
    return nullptr;
  }

  return algo;
}

MazeSearchAlgo::MazeSearchAlgo(AutorouteEngine* engine, const AutorouteControl& ctrl)
  : autorouteEngine(engine),
    control(ctrl),
    destinationDoor(nullptr),
    sectionNoOfDestinationDoor(0) {

  // Create destination distance calculator
  destinationDistance = std::make_unique<DestinationDistance>(
    ctrl.traceCosts,
    ctrl.layerActive,
    ctrl.minNormalViaCost,
    ctrl.minCheapViaCost
  );
}

bool MazeSearchAlgo::init(
    const std::vector<Item*>& startItems,
    const std::vector<Item*>& destItems) {

  if (startItems.empty() || destItems.empty()) {
    return false;
  }

  // Mark start items
  for (Item* item : startItems) {
    item->getAutorouteInfo().setStartInfo(true);
  }

  // Mark destination items and add to destination distance
  for (Item* item : destItems) {
    item->getAutorouteInfo().setStartInfo(false);

    // Add item bounding box to destination distance calculator
    IntBox bbox = item->getBoundingBox();
    int layer = 0; // Simplified - would need actual layer from item

    destinationDistance->join(bbox, layer);
  }

  // Initialize expansion from start items
  // In a full implementation, this would:
  // 1. Get expansion rooms containing start items
  // 2. Add their doors to the maze expansion list
  // 3. Calculate initial costs

  // For now, simplified placeholder
  return true;
}

MazeSearchAlgo::Result MazeSearchAlgo::findConnection() {
  Result result;

  int maxIterations = control.maxIterations;
  int iterations = 0;

  // A* search through expansion rooms
  while (!mazeExpansionList.empty() && iterations < maxIterations) {
    ++iterations;

    // Check if we should stop
    if (autorouteEngine->isStopRequested()) {
      break;
    }

    // Get lowest cost element
    MazeListElement current = mazeExpansionList.top();
    mazeExpansionList.pop();

    // Check if this element's maze search info is already occupied
    auto& mazeInfo = current.door->getMazeSearchElement(current.sectionNo);
    if (mazeInfo.isOccupied) {
      continue; // Already processed
    }

    // Mark as occupied
    mazeInfo.isOccupied = true;

    // Check if we reached a destination
    if (isDestination(current.door)) {
      destinationDoor = current.door;
      sectionNoOfDestinationDoor = current.sectionNo;
      result = reconstructPath();
      result.found = true;
      return result;
    }

    // Expand to neighbors
    expand(current);
  }

  return result;
}

void MazeSearchAlgo::expand(const MazeListElement& element) {
  // Expansion logic would:
  // 1. Get the next room from the element
  // 2. Find all doors in that room
  // 3. For each door, calculate cost and add to priority queue
  // 4. CHECK RULE AREAS to ensure routing is allowed

  if (!element.nextRoom) {
    return;
  }

  // Get doors from the room
  auto doors = element.nextRoom->getDoors();

  for (auto* door : doors) {
    if (!door) continue;

    // Get the other room through this door
    ExpansionRoom* otherRoom = door->otherRoom(element.nextRoom);
    if (!otherRoom) continue;

    // Expand through each section of the door
    int sectionCount = door->mazeSearchElementCount();
    for (int section = 0; section < sectionCount; ++section) {
      auto& mazeInfo = door->getMazeSearchElement(section);

      // Skip if already occupied
      if (mazeInfo.isOccupied) {
        continue;
      }

      // Get the door location for rule area checking
      // (Simplified - would use actual door center point from geometry)
      IntPoint doorLocation(0, 0);
      int doorLayer = otherRoom->getLayer();

      // CHECK RULE AREAS: Skip expansion if prohibited by rule area
      // This is the key integration point!
      if (!isExpansionAllowed(doorLocation, doorLayer)) {
        continue;  // Don't expand through prohibited areas
      }

      // Calculate cost to expand to this door/section
      double cost = calculateExpansionCost(element, door, section, otherRoom);

      // Create new expansion element
      MazeListElement newElement;
      newElement.door = door;
      newElement.sectionNo = section;
      newElement.nextRoom = otherRoom;
      newElement.layer = doorLayer;
      newElement.backtrackCost = element.backtrackCost + cost;

      // Calculate heuristic distance to destination
      double heuristic = destinationDistance->calculate(doorLocation, newElement.layer);

      newElement.sortingValue = newElement.backtrackCost + heuristic;

      // Set backtrack info
      mazeInfo.backtrackDoor = element.door;
      mazeInfo.sectionNoOfBacktrackDoor = element.sectionNo;

      // Add to priority queue
      mazeExpansionList.push(newElement);
    }
  }
}

double MazeSearchAlgo::calculateExpansionCost(
    const MazeListElement& from,
    ExpandableObject* toDoor,
    int toSection,
    ExpansionRoom* toRoom) const {

  // Base cost depends on trace width and clearance
  double baseCost = 100.0;

  // Add layer change cost if needed
  if (toRoom->getLayer() != from.layer) {
    baseCost += control.minNormalViaCost;
  }

  // Apply trace cost factor for the layer
  int layer = toRoom->getLayer();
  if (layer >= 0 && layer < static_cast<int>(control.traceCosts.size())) {
    // Use average of horizontal and vertical costs
    const auto& costs = control.traceCosts[layer];
    baseCost *= (costs.horizontal + costs.vertical) / 2.0;
  }

  return baseCost;
}

MazeSearchAlgo::Result MazeSearchAlgo::reconstructPath() {
  Result result;
  result.found = true;
  result.destinationDoor = destinationDoor;
  result.sectionNo = sectionNoOfDestinationDoor;

  // Backtrack from destination to start
  ExpandableObject* currentDoor = destinationDoor;
  int currentSection = sectionNoOfDestinationDoor;

  constexpr int kMaxPathLength = 10000;
  int pathLength = 0;

  while (currentDoor && pathLength < kMaxPathLength) {
    ++pathLength;

    // Get maze info for backtracking
    const auto& mazeInfo = currentDoor->getMazeSearchElement(currentSection);

    // Add current point to path
    // (Simplified - would use actual door center)
    IntPoint point(0, 0);
    result.pathPoints.push_back(point);
    result.pathLayers.push_back(0); // Simplified layer

    // Move to backtrack door
    if (!mazeInfo.backtrackDoor) {
      break; // Reached start
    }

    currentDoor = mazeInfo.backtrackDoor;
    currentSection = mazeInfo.sectionNoOfBacktrackDoor;
  }

  // Reverse path to go from start to destination
  std::reverse(result.pathPoints.begin(), result.pathPoints.end());
  std::reverse(result.pathLayers.begin(), result.pathLayers.end());

  return result;
}

bool MazeSearchAlgo::isDestination(ExpandableObject* door) const {
  // Check if this is a target item expansion door
  auto* targetDoor = dynamic_cast<TargetItemExpansionDoor*>(door);
  if (!targetDoor) {
    return false;
  }

  // Check if it's a destination (not a start)
  return targetDoor->isDestinationDoor();
}

bool MazeSearchAlgo::isExpansionAllowed(IntPoint point, int layer) const {
  // Check if board has rule areas that prohibit this expansion
  auto* board = autorouteEngine->board;
  if (!board) {
    return true;  // No board, allow expansion
  }

  // Get the net being routed
  int netNo = autorouteEngine->getNetNo();
  if (netNo <= 0) {
    return true;  // No net, allow expansion
  }

  // Check if traces are prohibited at this location
  // This is the enforcement point for KiCad rule areas!
  if (board->isTraceProhibited(point, layer, netNo)) {
    return false;  // Rule area prohibits routing here
  }

  return true;  // Expansion is allowed
}

} // namespace freerouting
