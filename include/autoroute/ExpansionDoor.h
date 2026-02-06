#ifndef FREEROUTING_AUTOROUTE_EXPANSIONDOOR_H
#define FREEROUTING_AUTOROUTE_EXPANSIONDOOR_H

#include "autoroute/ExpandableObject.h"
#include "autoroute/ExpansionRoom.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/Shape.h"
#include <vector>
#include <memory>

namespace freerouting {

// Forward declaration
class CompleteExpansionRoom;

// An ExpansionDoor is a common edge between two ExpansionRooms
// Doors represent the boundary where routing can cross from one room to another
class ExpansionDoor : public ExpandableObject {
public:
  // The two rooms connected by this door
  ExpansionRoom* firstRoom;
  ExpansionRoom* secondRoom;

  // The dimension of a door may be 1 (line) or 2 (area)
  int dimension;

  // Creates a new ExpansionDoor between two rooms
  ExpansionDoor(ExpansionRoom* first, ExpansionRoom* second, int dim)
    : firstRoom(first), secondRoom(second), dimension(dim),
      cachedShape(nullptr) {}

  // Creates a new ExpansionDoor with auto-calculated dimension
  ExpansionDoor(ExpansionRoom* first, ExpansionRoom* second);

  ~ExpansionDoor() override = default;

  // Calculate intersection of the shapes of the 2 rooms belonging to this door
  const Shape* getShape() const override;

  // Get the dimension of this door (1 or 2)
  int getDimension() const override {
    return dimension;
  }

  // Returns the other room of this door
  // Returns nullptr if room is neither first_room nor second_room
  ExpansionRoom* otherRoom(ExpansionRoom* room);

  // Returns the other room if it is a CompleteExpansionRoom, else nullptr
  CompleteExpansionRoom* otherRoom(CompleteExpansionRoom* room) override;

  // MazeSearchElement access
  int mazeSearchElementCount() const override {
    return static_cast<int>(sectionArray.size());
  }

  MazeSearchElement& getMazeSearchElement(int no) override {
    return sectionArray[no];
  }

  const MazeSearchElement& getMazeSearchElement(int no) const override {
    return sectionArray[no];
  }

  // Resets this ExpandableObject for autorouting the next connection
  void reset() override;

  // Allocates and initializes sectionCount sections
  void allocateSections(int sectionCount);

protected:
  // Each section can be expanded separately by the maze search algorithm
  std::vector<MazeSearchElement> sectionArray;

  // Cached shape (intersection of first and second room shapes)
  mutable const Shape* cachedShape;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_EXPANSIONDOOR_H
