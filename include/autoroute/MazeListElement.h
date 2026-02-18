#ifndef FREEROUTING_AUTOROUTE_MAZELISTELEMENT_H
#define FREEROUTING_AUTOROUTE_MAZELISTELEMENT_H

#include "autoroute/ExpandableObject.h"
#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/Vector2.h"
#include <deque>

namespace freerouting {

// Information for the maze expand algorithm contained in expansion doors and drills
// Uses object pooling to reduce memory allocation during pathfinding
class MazeListElement {
public:
  // The door or drill belonging to this MazeListElement
  ExpandableObject* door;

  // The section number of the door (or the layer of the drill)
  int sectionNoOfDoor;

  // The door from which this door was expanded
  ExpandableObject* backtrackDoor;

  // The section number of the backtrack door
  int sectionNoOfBacktrackDoor;

  // The weighted distance to the start of the expansion
  double expansionValue;

  // The expansion value plus the shortest distance to a destination
  // The list is sorted in ascending order by this value
  double sortingValue;

  // The next room which will be expanded from this maze search element
  CompleteExpansionRoom* nextRoom;

  // Point of the region of the expansion door which has the shortest distance to the backtrack door
  // TODO: Implement FloatLine properly or use pair of FloatPoints
  FloatPoint shapeEntry;

  bool roomRipped;
  MazeSearchElement::Adjustment adjustment;
  bool alreadyChecked;

  // Obtain a MazeListElement from the pool or create a new one if pool is empty
  static MazeListElement* obtain(
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
    bool pAlreadyChecked);

  // Recycle this element back to the pool for reuse
  static void recycle(MazeListElement* element);

  // Comparison for priority queue (lower sorting value = higher priority)
  bool operator>(const MazeListElement& other) const {
    return sortingValue > other.sortingValue;
  }

  bool operator<(const MazeListElement& other) const {
    return sortingValue < other.sortingValue;
  }

private:
  // Thread-local pool for recycling instances
  static thread_local std::deque<MazeListElement*> pool;
  static constexpr int maxPoolSize = 500;  // Cap pool size to prevent unbounded growth

  // Private constructor for pooling
  MazeListElement()
    : door(nullptr),
      sectionNoOfDoor(0),
      backtrackDoor(nullptr),
      sectionNoOfBacktrackDoor(0),
      expansionValue(0.0),
      sortingValue(0.0),
      nextRoom(nullptr),
      shapeEntry(),
      roomRipped(false),
      adjustment(MazeSearchElement::Adjustment::None),
      alreadyChecked(false) {}
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_MAZELISTELEMENT_H
