#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/FreeSpaceExpansionRoom.h"
#include "autoroute/ObstacleExpansionRoom.h"
#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/MazeListElement.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/DrillPage.h"
#include "board/Item.h"
#include "board/RoutingBoard.h"
#include "board/Pin.h"
#include "board/Via.h"
#include "board/Trace.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>

namespace freerouting {

// Constants
static constexpr int ALREADY_RIPPED_COSTS = 1;

// Simplified FloatLine for maze algorithm (until full geometry is ported)
struct FloatLine {
  FloatPoint a;
  FloatPoint b;

  FloatLine() : a(), b() {}
  FloatLine(FloatPoint pa, FloatPoint pb) : a(pa), b(pb) {}

  FloatPoint middlePoint() const {
    return FloatPoint((a.x + b.x) / 2.0, (a.y + b.y) / 2.0);
  }

  double distanceSquare() const {
    double dx = b.x - a.x;
    double dy = b.y - a.y;
    return dx * dx + dy * dy;
  }
};

// MazeExpansionList implementation - priority queue wrapper
void MazeSearchAlgo::MazeExpansionList::add(MazeListElement* element) {
  queue.push_back(element);
  // Bubble up to maintain heap property (min heap based on sortingValue)
  int i = static_cast<int>(queue.size()) - 1;
  while (i > 0) {
    int parent = (i - 1) / 2;
    if (queue[i]->sortingValue >= queue[parent]->sortingValue) {
      break;
    }
    std::swap(queue[i], queue[parent]);
    i = parent;
  }
}

MazeListElement* MazeSearchAlgo::MazeExpansionList::poll() {
  if (queue.empty()) {
    return nullptr;
  }

  MazeListElement* result = queue[0];
  queue[0] = queue.back();
  queue.pop_back();

  if (queue.empty()) {
    return result;
  }

  // Bubble down to maintain heap property
  int i = 0;
  int size = static_cast<int>(queue.size());
  while (true) {
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int smallest = i;

    if (left < size && queue[left]->sortingValue < queue[smallest]->sortingValue) {
      smallest = left;
    }
    if (right < size && queue[right]->sortingValue < queue[smallest]->sortingValue) {
      smallest = right;
    }
    if (smallest == i) {
      break;
    }
    std::swap(queue[i], queue[smallest]);
    i = smallest;
  }

  return result;
}

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

  // Store start and dest items for SimpleGridRouter fallback
  this->startItems = startItems;
  this->destItems = destItems;

  // Mark start items
  for (Item* item : startItems) {
    item->getAutorouteInfo().setStartInfo(true);
  }

  // Mark destination items and add to destination distance
  for (Item* item : destItems) {
    item->getAutorouteInfo().setStartInfo(false);

    // Add item bounding box to destination distance calculator
    IntBox bbox = item->getBoundingBox();
    int layer = item->firstLayer();

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
  // Main maze search algorithm entry point
  // Line-by-line port from Java MazeSearchAlgo.find_connection() lines 204-212

  while (occupyNextElement()) {
    // Continue expanding
  }

  Result result;
  if (this->destinationDoor == nullptr) {
    result.found = false;
    return result;
  }

  result.found = true;
  result.destinationDoor = this->destinationDoor;
  result.sectionNo = this->sectionNoOfDestinationDoor;
  return result;
}

// Port of Java MazeSearchAlgo.occupy_next_element() lines 218-292
bool MazeSearchAlgo::occupyNextElement() {
  if (this->destinationDoor != nullptr) {
    return false; // destination already reached
  }

  MazeListElement* listElement = nullptr;
  MazeSearchElement* currDoorSection = nullptr;

  // Search the next element, which is not yet expanded
  // Use poll() to efficiently get and remove the best element (O(log n))
  bool nextElementFound = false;
  while (!mazeExpansionList.empty()) {
    if (this->autorouteEngine->isStopRequested()) {
      return false;
    }

    listElement = mazeExpansionList.poll(); // Gets highest priority element
    if (listElement == nullptr) {
      break; // Queue unexpectedly empty
    }

    int currSectionNo = listElement->sectionNoOfDoor;
    currDoorSection = &(listElement->door->getMazeSearchElement(currSectionNo));

    if (!currDoorSection->isOccupied) {
      nextElementFound = true;
      break;
    }

    // Element already occupied, recycle and continue
    MazeListElement::recycle(listElement);
  }

  if (!nextElementFound) {
    return false;
  }

  currDoorSection->backtrackDoor = listElement->backtrackDoor;
  currDoorSection->sectionNoOfBacktrackDoor = listElement->sectionNoOfBacktrackDoor;
  currDoorSection->roomRipped = listElement->roomRipped;
  currDoorSection->adjustment = listElement->adjustment;

  // Check if this is a drill page - expand to drills
  auto* drillPage = dynamic_cast<DrillPage*>(listElement->door);
  if (drillPage != nullptr) {
    expandToDrillsOfPage(listElement);
    return true;
  }

  // Check if we reached destination
  auto* currTargetDoor = dynamic_cast<TargetItemExpansionDoor*>(listElement->door);
  if (currTargetDoor != nullptr) {
    if (currTargetDoor->isDestinationDoor()) {
      // The destination is reached
      this->destinationDoor = currTargetDoor;
      this->sectionNoOfDestinationDoor = listElement->sectionNoOfDoor;
      return false;
    }
  }

  // Check for fanout completion
  auto* currDrill = dynamic_cast<ExpansionDrill*>(listElement->door);
  auto* backtrackDrill = dynamic_cast<ExpansionDrill*>(listElement->backtrackDoor);
  if (control.isFanout && currDrill != nullptr && backtrackDrill != nullptr) {
    // algorithm completed after the first drill
    this->destinationDoor = listElement->door;
    this->sectionNoOfDestinationDoor = listElement->sectionNoOfDoor;
    return false;
  }

  // Expand to other layers via vias
  if (control.viasAllowed && currDrill != nullptr && backtrackDrill == nullptr) {
    expandToOtherLayers(listElement);
  }

  // Expand to room doors
  if (listElement->nextRoom != nullptr) {
    if (!expandToRoomDoors(listElement)) {
      return true; // occupation by ripup is delayed
    }
  }

  currDoorSection->isOccupied = true;
  return true;
}

// Port of Java MazeSearchAlgo.expand_to_room_doors() lines 298-469
bool MazeSearchAlgo::expandToRoomDoors(MazeListElement* pListElement) {
  // Complete the neighbour rooms to make sure doors won't change
  int layerNo = pListElement->nextRoom->getLayer();

  bool layerActive = control.layerActive[layerNo];
  if (!layerActive) {
    // Check if signal layer
    // TODO: Need to check board->layerStructure.arr[layerNo].isSignal
    // For now, assume non-active means we should return true
    return true;
  }

  double halfWidth = control.compensatedTraceHalfWidth[layerNo];
  bool currDoorIsSmall = false;

  auto* currExpDoor = dynamic_cast<ExpansionDoor*>(pListElement->door);
  if (currExpDoor != nullptr) {
    double halfWidthAdd = halfWidth + AutorouteEngine::TRACE_WIDTH_TOLERANCE;
    // TODO: with_neckdown check - skipping for now
    currDoorIsSmall = doorIsSmall(currExpDoor, 2 * halfWidthAdd);
  }

  // Complete neighbour rooms
  autorouteEngine->completeNeighbourRooms(pListElement->nextRoom);

  FloatPoint shapeEntryMiddle = pListElement->shapeEntry;

  // TODO: neckdown at start pin - skipping for now

  bool nextRoomIsThick = true;
  auto* obstacleRoom = dynamic_cast<ObstacleExpansionRoom*>(pListElement->nextRoom);
  if (obstacleRoom != nullptr) {
    nextRoomIsThick = roomShapeIsThick(obstacleRoom);
  } else {
    // const Shape* nextRoomShape = pListElement->nextRoom->getShape();
    // TODO: Implement minWidth() for Shape
    if (false) {  // nextRoomShape->minWidth() < 2 * halfWidth
      nextRoomIsThick = false;
    } else if (!pListElement->alreadyChecked && pListElement->door->getDimension() == 1 && !currDoorIsSmall) {
      // Check if room is thick at entry point
      // TODO: Use nearest_border_points_approx when Shape is fully ported
      // For now, assume thick
      nextRoomIsThick = true;
    }
  }

  // Check drill to foreign conduction area on split plane
  if (!layerActive) {
    auto* drill = dynamic_cast<ExpansionDrill*>(pListElement->door);
    if (drill != nullptr) {
      // TODO: Check picked items - skipping for now
    }
  }

  bool somethingExpanded = expandToTargetDoors(pListElement, nextRoomIsThick, currDoorIsSmall, shapeEntryMiddle);

  if (!layerActive) {
    return true;
  }

  int ripupCosts = 0;

  auto* freeSpaceRoom = dynamic_cast<FreeSpaceExpansionRoom*>(pListElement->nextRoom);
  if (freeSpaceRoom != nullptr) {
    if (!pListElement->alreadyChecked) {
      if (currDoorIsSmall) {
        bool enterThroughSmallDoor = false;
        if (nextRoomIsThick) {
          // Check leaving ripped item
          enterThroughSmallDoor = checkLeavingRippedItem(pListElement);
        }
        if (!enterThroughSmallDoor) {
          return somethingExpanded;
        }
      }
    }
  } else if (obstacleRoom != nullptr) {
    if (!pListElement->alreadyChecked) {
      bool roomRippable = false;
      if (this->control.ripupAllowed) {
        ripupCosts = checkRipup(pListElement, obstacleRoom->getItem(), currDoorIsSmall);
        roomRippable = ripupCosts >= 0;
      }

      if (ripupCosts != ALREADY_RIPPED_COSTS && nextRoomIsThick) {
        // Item* obstacleItem = obstacleRoom->getItem();
        // TODO: Implement shoving logic
        // if (!currDoorIsSmall && control.maxShoveTraceRecursionDepth > 0 && dynamic_cast<Trace*>(obstacleItem)) {
        //   bool shoved = shoveTraceRoom(pListElement, obstacleRoom);
        //   ...
        // }
      }

      if (!roomRippable) {
        return true;
      }
    }
  }

  // Expand to doors of the room
  auto& doors = pListElement->nextRoom->getDoors();
  for (auto* toDoor : doors) {
    if (toDoor == pListElement->door) {
      continue;
    }
    if (expandToDoor(toDoor, pListElement, ripupCosts, nextRoomIsThick, MazeSearchElement::Adjustment::None)) {
      somethingExpanded = true;
    }
  }

  // TODO: Expand to drill pages - requires DrillPageArray infrastructure
  // Skipping for now

  return somethingExpanded;
}

// Port of Java MazeSearchAlgo::expand_to_door() lines 543-588
bool MazeSearchAlgo::expandToDoor(ExpansionDoor* pToDoor, MazeListElement* pListElement,
                                   int pAddCosts, bool pNextRoomIsThick,
                                   MazeSearchElement::Adjustment pAdjustment) {
  // double halfWidth = control.compensatedTraceHalfWidth[pListElement->nextRoom->getLayer()];
  bool somethingExpanded = false;

  // TODO: Get section segments - requires full geometry port
  // For now, use simplified approach with one section
  int sectionCount = pToDoor->mazeSearchElementCount();

  for (int i = 0; i < sectionCount; ++i) {
    if (pToDoor->getMazeSearchElement(i).isOccupied) {
      continue;
    }

    FloatLine newShapeEntry;
    if (pNextRoomIsThick) {
      // TODO: Use actual line sections from door geometry
      // For now, use point approximation
      newShapeEntry.a = pListElement->shapeEntry;
      newShapeEntry.b = pListElement->shapeEntry;
    } else {
      // Expand only doors on opposite side
      // TODO: Implement segment_projection
      newShapeEntry.a = pListElement->shapeEntry;
      newShapeEntry.b = pListElement->shapeEntry;
    }

    if (expandToDoorSection(pToDoor, i, newShapeEntry, pListElement, pAddCosts, pAdjustment)) {
      somethingExpanded = true;
    }
  }

  return somethingExpanded;
}

// Port of Java MazeSearchAlgo.expand_to_door_section() lines 623-645
bool MazeSearchAlgo::expandToDoorSection(ExpandableObject* pDoor, int pSectionNo,
                                          const FloatLine& pShapeEntry, MazeListElement* pFromElement,
                                          int pAddCosts, MazeSearchElement::Adjustment pAdjustment) {
  if (pDoor->getMazeSearchElement(pSectionNo).isOccupied) {
    return false;
  }

  CompleteExpansionRoom* nextRoom = pDoor->otherRoom(pFromElement->nextRoom);
  int layer = pFromElement->nextRoom->getLayer();
  FloatPoint shapeEntryMiddle((pShapeEntry.a.x + pShapeEntry.b.x) / 2.0,
                               (pShapeEntry.a.y + pShapeEntry.b.y) / 2.0);
  FloatPoint fromShapeEntryMiddle = pFromElement->shapeEntry;

  double dx = shapeEntryMiddle.x - fromShapeEntryMiddle.x;
  double dy = shapeEntryMiddle.y - fromShapeEntryMiddle.y;
  double distance = std::sqrt(dx * dx + dy * dy);

  // Use weighted distance with trace costs
  double weightedDist = distance;
  if (layer >= 0 && layer < static_cast<int>(control.traceCosts.size())) {
    double avgCost = (control.traceCosts[layer].horizontal + control.traceCosts[layer].vertical) / 2.0;
    weightedDist *= avgCost;
  }

  double expansionValue = pFromElement->expansionValue + pAddCosts + weightedDist;
  double sortingValue = expansionValue + destinationDistance->calculate(shapeEntryMiddle, layer);

  bool roomRipped = (pAddCosts > 0 && pAdjustment == MazeSearchElement::Adjustment::None) ||
                     (pFromElement->alreadyChecked && pFromElement->roomRipped);

  MazeListElement* newElement = MazeListElement::obtain(
    pDoor, pSectionNo, pFromElement->door, pFromElement->sectionNoOfDoor,
    expansionValue, sortingValue, nextRoom, shapeEntryMiddle,  // Use FloatPoint
    roomRipped, pAdjustment, false
  );

  mazeExpansionList.add(newElement);
  return true;
}

// Port of Java MazeSearchAlgo.expand_to_other_layers() lines 748-880
void MazeSearchAlgo::expandToOtherLayers(MazeListElement* pListElement) {
  // TODO: Full via expansion requires drill infrastructure
  // This is a simplified stub for now
  // Will be completed when ExpansionDrill and via checking is fully ported
  (void)pListElement;
}

// Port of Java MazeSearchAlgo.check_ripup() lines 1003-1085
int MazeSearchAlgo::checkRipup(MazeListElement* pListElement, Item* pObstacleItem, bool pDoorIsSmall) {
  if (!pObstacleItem->isRoutable()) {
    return -1;
  }

  if (pDoorIsSmall) {
    // Allow entering via or trace if border segment is smaller than trace width
    if (!enterThroughSmallDoor(pListElement, pObstacleItem)) {
      return -1;
    }
  }

  CompleteExpansionRoom* previousRoom = pListElement->door->otherRoom(pListElement->nextRoom);
  bool roomWasShoved = pListElement->adjustment != MazeSearchElement::Adjustment::None;
  Item* previousItem = nullptr;

  auto* prevObstacleRoom = dynamic_cast<ObstacleExpansionRoom*>(previousRoom);
  if (prevObstacleRoom != nullptr) {
    previousItem = prevObstacleRoom->getItem();
  }

  if (roomWasShoved) {
    if (previousItem != nullptr && previousItem != pObstacleItem) {
      // Check if they share a net
      const auto& prevNets = previousItem->getNets();
      const auto& obstNets = pObstacleItem->getNets();
      bool sharesNet = false;
      for (int net : prevNets) {
        if (std::find(obstNets.begin(), obstNets.end(), net) != obstNets.end()) {
          sharesNet = true;
          break;
        }
      }
      if (sharesNet) {
        // The ripped trace may start at a fork
        return -1;
      }
    }
  } else if (previousItem == pObstacleItem) {
    return ALREADY_RIPPED_COSTS;
  }

  double fanoutViaCostFactor = 1.0;
  double costFactor = 1.0;

  auto* obstacleTrace = dynamic_cast<Trace*>(pObstacleItem);
  if (obstacleTrace != nullptr) {
    costFactor = obstacleTrace->getHalfWidth();
    if (!this->control.removeUnconnectedVias) {
      // Protect traces between SMD-pins and fanout vias
      fanoutViaCostFactor = calcFanoutViaRipupCostFactor(obstacleTrace);
    }
  } else {
    auto* obstacleVia = dynamic_cast<Via*>(pObstacleItem);
    if (obstacleVia != nullptr) {
      // bool lookIfFanoutVia = !this->control.removeUnconnectedVias;
      // TODO: Check via contacts and calculate cost
      // Simplified for now
      costFactor = 10.0;
    }
  }

  double ripupCost = this->control.ripupCosts * costFactor;
  double detour = 1.0;

  if (fanoutViaCostFactor <= 1.0) {
    // TODO: Get connection and detour
    // Connection* obstacleConnection = Connection::get(pObstacleItem);
    // if (obstacleConnection != nullptr) {
    //   detour = obstacleConnection->getDetour();
    // }
  }

  bool randomize = this->control.ripupPassNo >= 4 && this->control.ripupPassNo % 3 != 0;
  if (randomize) {
    // Shuffle result to avoid repetitive loops
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double randomNumber = dis(gen);
    double randomFactor = 0.5 + randomNumber * randomNumber;
    detour *= randomFactor;
  }

  ripupCost /= detour;
  ripupCost *= fanoutViaCostFactor;

  int result = std::max(static_cast<int>(ripupCost), 1);
  constexpr int MAX_RIPUP_COSTS = INT_MAX / 100;
  return std::min(result, MAX_RIPUP_COSTS);
}

// Helper methods

bool MazeSearchAlgo::doorIsSmall(ExpansionDoor* pDoor, double pTraceWidth) {
  if (pDoor->dimension == 1) {
    const Shape* doorShape = pDoor->getShape();
    if (doorShape->isEmpty()) {
      return true;
    }

    // TODO: Implement maxWidth() for Shape
    double doorLength = 100.0; // doorShape->maxWidth();
    // TODO: Use angle restriction from board
    // For now, use simple max width check
    return doorLength < pTraceWidth;
  }
  return false;
}

bool MazeSearchAlgo::roomShapeIsThick(ObstacleExpansionRoom* pObstacleRoom) {
  Item* obstacleItem = pObstacleRoom->getItem();
  int layer = pObstacleRoom->getLayer();
  double obstacleHalfWidth = 0;

  auto* trace = dynamic_cast<Trace*>(obstacleItem);
  if (trace != nullptr) {
    obstacleHalfWidth = trace->getHalfWidth();
    // TODO: Add clearance compensation
  } else {
    auto* via = dynamic_cast<Via*>(obstacleItem);
    if (via != nullptr) {
      // TODO: Get via shape on layer
      obstacleHalfWidth = 50.0; // Simplified
    }
  }

  return obstacleHalfWidth >= this->control.compensatedTraceHalfWidth[layer];
}

double MazeSearchAlgo::calcFanoutViaRipupCostFactor(Trace* pTrace) {
  // Port of Java MazeSearchAlgo.calc_fanout_via_ripup_cost_factor() lines 123-158
  // constexpr double FANOUT_COST_CONST = 20000.0;

  // TODO: Check trace contacts and fanout detection
  // For now, return 1.0 (no fanout protection)
  (void)pTrace;
  return 1.0;
}

bool MazeSearchAlgo::enterThroughSmallDoor(MazeListElement* pListElement, Item* pIgnoreItem) {
  // Port of Java MazeSearchAlgo.enter_through_small_door() lines 1167-1225
  // TODO: Implement full door checking logic
  // For now, allow entry
  (void)pListElement;
  (void)pIgnoreItem;
  return true;
}

bool MazeSearchAlgo::checkLeavingRippedItem(MazeListElement* pListElement) {
  // Port of Java MazeSearchAlgo.check_leaving_ripped_item() lines 1231-1244
  auto* currDoor = dynamic_cast<ExpansionDoor*>(pListElement->door);
  if (currDoor == nullptr) {
    return false;
  }

  CompleteExpansionRoom* fromRoom = currDoor->otherRoom(pListElement->nextRoom);
  auto* obstacleRoom = dynamic_cast<ObstacleExpansionRoom*>(fromRoom);
  if (obstacleRoom == nullptr) {
    return false;
  }

  Item* currItem = obstacleRoom->getItem();
  if (!currItem->isRoutable()) {
    return false;
  }

  return enterThroughSmallDoor(pListElement, currItem);
}

bool MazeSearchAlgo::expandToTargetDoors(MazeListElement* pListElement, bool pNextRoomIsThick,
                                          bool pCurrDoorIsSmall, const FloatPoint& pShapeEntryMiddle) {
  // Port of Java MazeSearchAlgo.expand_to_target_doors() lines 475-538
  (void)pNextRoomIsThick;  // Used in full implementation
  if (pCurrDoorIsSmall) {
    bool enterThroughSmallDoor = false;
    auto* expDoor = dynamic_cast<ExpansionDoor*>(pListElement->door);
    if (expDoor != nullptr) {
      CompleteExpansionRoom* fromRoom = expDoor->otherRoom(pListElement->nextRoom);
      auto* obstRoom = dynamic_cast<ObstacleExpansionRoom*>(fromRoom);
      if (obstRoom != nullptr) {
        enterThroughSmallDoor = true;
      }
    }
    if (!enterThroughSmallDoor) {
      return false;
    }
  }

  bool result = false;
  auto& targetDoors = pListElement->nextRoom->getTargetDoors();

  for (auto* toDoor : targetDoors) {
    if (toDoor == pListElement->door) {
      continue;
    }

    // TODO: Validate tree shape index and get target shape
    // For now, use simplified approach
    FloatPoint connectionPoint = pShapeEntryMiddle;
    FloatLine newShapeEntry(connectionPoint, connectionPoint);

    if (expandToDoorSection(toDoor, 0, newShapeEntry, pListElement, 0, MazeSearchElement::Adjustment::None)) {
      result = true;
    }
  }

  return result;
}

void MazeSearchAlgo::expandToDrillsOfPage(MazeListElement* pFromElement) {
  // Port of Java MazeSearchAlgo.expand_to_drills_of_page() lines 729-743
  // TODO: Requires DrillPage infrastructure
  // Stub for now
  (void)pFromElement;
}

} // namespace freerouting
