#include "autoroute/MazeListElement.h"

namespace freerouting {

// Thread-local pool initialization
thread_local std::deque<MazeListElement*> MazeListElement::pool;

MazeListElement* MazeListElement::obtain(
    ExpandableObject* pDoor,
    int pSectionNoOfDoor,
    ExpandableObject* pBacktrackDoor,
    int pSectionNoOfBacktrackDoor,
    double pExpansionValue,
    double pSortingValue,
    CompleteExpansionRoom* pNextRoom,
    FloatPoint pShapeEntry,
    bool pRoomRipped,
    MazeSearchElement::Adjustment pAdjustment,
    bool pAlreadyChecked) {

  MazeListElement* element = nullptr;

  if (!pool.empty()) {
    element = pool.front();
    pool.pop_front();
  } else {
    element = new MazeListElement();
  }

  // Initialize/reset fields
  element->door = pDoor;
  element->sectionNoOfDoor = pSectionNoOfDoor;
  element->backtrackDoor = pBacktrackDoor;
  element->sectionNoOfBacktrackDoor = pSectionNoOfBacktrackDoor;
  element->expansionValue = pExpansionValue;
  element->sortingValue = pSortingValue;
  element->nextRoom = pNextRoom;
  element->shapeEntry = pShapeEntry;
  element->roomRipped = pRoomRipped;
  element->adjustment = pAdjustment;
  element->alreadyChecked = pAlreadyChecked;

  return element;
}

void MazeListElement::recycle(MazeListElement* element) {
  if (!element) {
    return;
  }

  if (static_cast<int>(pool.size()) < maxPoolSize) {
    // Clear references to help with memory management
    element->door = nullptr;
    element->backtrackDoor = nullptr;
    element->nextRoom = nullptr;
    element->adjustment = MazeSearchElement::Adjustment::None;

    pool.push_back(element);
  } else {
    // Pool is full, delete element
    delete element;
  }
}

} // namespace freerouting
