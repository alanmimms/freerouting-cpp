#ifndef FREEROUTING_AUTOROUTE_TARGETITEMEXPANSIONDOOR_H
#define FREEROUTING_AUTOROUTE_TARGETITEMEXPANSIONDOOR_H

#include "autoroute/ExpandableObject.h"
#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/MazeSearchElement.h"
#include "board/Item.h"
#include "geometry/Shape.h"

namespace freerouting {

// Forward declaration
class ShapeSearchTree;

// An expansion door leading to a start or destination item of the autoroute algorithm
// Represents the connection point between a routing area and a target pad/pin
class TargetItemExpansionDoor : public ExpandableObject {
public:
  Item* const item;
  int const treeEntryNo;
  CompleteExpansionRoom* const room;

  // Creates a new TargetItemExpansionDoor
  TargetItemExpansionDoor(Item* targetItem, int entryNo,
                          CompleteExpansionRoom* expansionRoom,
                          const Shape* doorShape)
    : item(targetItem),
      treeEntryNo(entryNo),
      room(expansionRoom),
      shape(doorShape) {}

  ~TargetItemExpansionDoor() override = default;

  // Get the shape (intersection of item and room)
  const Shape* getShape() const override {
    return shape;
  }

  // Target doors are always 2-dimensional
  int getDimension() const override {
    return 2;
  }

  // Check if this is a destination door (vs start door)
  bool isDestinationDoor() const;

  // Target doors don't lead to other rooms
  CompleteExpansionRoom* otherRoom(CompleteExpansionRoom* /*room*/) override {
    return nullptr;
  }

  // Target doors have exactly one maze search element
  int mazeSearchElementCount() const override {
    return 1;
  }

  MazeSearchElement& getMazeSearchElement(int /*no*/) override {
    return mazeSearchInfo;
  }

  const MazeSearchElement& getMazeSearchElement(int /*no*/) const override {
    return mazeSearchInfo;
  }

  // Reset for next connection
  void reset() override {
    mazeSearchInfo.reset();
  }

private:
  const Shape* shape;
  MazeSearchElement mazeSearchInfo;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_TARGETITEMEXPANSIONDOOR_H
