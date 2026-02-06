#ifndef FREEROUTING_AUTOROUTE_MAZESEARCHELEMENT_H
#define FREEROUTING_AUTOROUTE_MAZESEARCHELEMENT_H

namespace freerouting {

// Forward declaration
class ExpandableObject;

// Describes the structure of a section of an ExpandableObject
// Used by the maze expansion algorithm for pathfinding
struct MazeSearchElement {
  enum class Adjustment {
    None,
    Right,
    Left
  };

  // True if this door is already occupied by the maze expanding algorithm
  bool isOccupied;

  // Used for backtracking in the maze expanding algorithm
  ExpandableObject* backtrackDoor;
  int sectionNoOfBacktrackDoor;

  bool roomRipped;
  Adjustment adjustment;

  MazeSearchElement()
    : isOccupied(false),
      backtrackDoor(nullptr),
      sectionNoOfBacktrackDoor(0),
      roomRipped(false),
      adjustment(Adjustment::None) {}

  // Resets this MazeSearchElement for autorouting the next connection
  void reset() {
    isOccupied = false;
    backtrackDoor = nullptr;
    sectionNoOfBacktrackDoor = 0;
    roomRipped = false;
    adjustment = Adjustment::None;
  }
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_MAZESEARCHELEMENT_H
