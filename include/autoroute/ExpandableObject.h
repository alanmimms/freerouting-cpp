#ifndef FREEROUTING_AUTOROUTE_EXPANDABLEOBJECT_H
#define FREEROUTING_AUTOROUTE_EXPANDABLEOBJECT_H

#include "autoroute/MazeSearchElement.h"
#include "geometry/Shape.h"

namespace freerouting {

// Forward declarations
class CompleteExpansionRoom;

// An object which can be expanded by the maze expansion algorithm
// This is the base interface for doors and target items
class ExpandableObject {
public:
  virtual ~ExpandableObject() = default;

  // Get the shape of this expandable object
  // For doors, this is the intersection of the two room shapes
  virtual const Shape* getShape() const = 0;

  // Returns the dimension of the intersection of the shapes
  // 0 = point, 1 = line, 2 = area
  virtual int getDimension() const = 0;

  // Returns the other room to room if this is a door and the other room is complete
  // Else nullptr is returned
  virtual CompleteExpansionRoom* otherRoom(CompleteExpansionRoom* room) = 0;

  // Returns the count of MazeSearchElements in this expandable object
  virtual int mazeSearchElementCount() const = 0;

  // Returns the p_no-th MazeSearchElement in this expandable object
  virtual MazeSearchElement& getMazeSearchElement(int no) = 0;
  virtual const MazeSearchElement& getMazeSearchElement(int no) const = 0;

  // Resets this ExpandableObject for autorouting the next connection
  virtual void reset() = 0;

protected:
  ExpandableObject() = default;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_EXPANDABLEOBJECT_H
