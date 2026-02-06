#ifndef FREEROUTING_AUTOROUTE_INCOMPLETEFREESPACEEXPANSIONROOM_H
#define FREEROUTING_AUTOROUTE_INCOMPLETEFREESPACEEXPANSIONROOM_H

#include "autoroute/FreeSpaceExpansionRoom.h"
#include "geometry/Shape.h"
#include <vector>

namespace freerouting {

class TargetItemExpansionDoor;

// An expansion room whose shape is not yet completely calculated
// Used during the expansion process before the final shape is determined
class IncompleteFreeSpaceExpansionRoom : public FreeSpaceExpansionRoom {
public:
  // Creates a new IncompleteFreeSpaceExpansionRoom
  // containedShape is a shape which should be contained in the completed shape
  IncompleteFreeSpaceExpansionRoom(const Shape* roomShape, int layerNum,
                                    const Shape* containedShape)
    : FreeSpaceExpansionRoom(roomShape, layerNum),
      containedShape(containedShape) {}

  ~IncompleteFreeSpaceExpansionRoom() override = default;

  // Get the shape that should be contained in the completed shape
  const Shape* getContainedShape() const {
    return containedShape;
  }

  // Set the contained shape
  void setContainedShape(const Shape* shape) {
    containedShape = shape;
  }

  // Incomplete rooms don't have target doors yet
  // Returns empty vector
  std::vector<TargetItemExpansionDoor*> getTargetDoors() const {
    return std::vector<TargetItemExpansionDoor*>();
  }

private:
  const Shape* containedShape;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_INCOMPLETEFREESPACEEXPANSIONROOM_H
