#include "autoroute/ObstacleExpansionRoom.h"
#include "board/Item.h"
#include "board/PolylineTrace.h"
#include "board/ShapeSearchTree.h"

namespace freerouting {

ObstacleExpansionRoom::ObstacleExpansionRoom(Item* obstacleItem, int indexInItem, ShapeSearchTree* shapeTree)
  : item(obstacleItem),
    indexInItem(indexInItem),
    roomShape(nullptr),
    doorsCalculated(false) {
  if (obstacleItem && shapeTree) {
    roomShape = obstacleItem->getTreeShape(shapeTree, indexInItem);
  }
}

int ObstacleExpansionRoom::getLayer() const {
  if (item) {
    return item->shapeLayer(indexInItem);
  }
  return 0;
}

bool ObstacleExpansionRoom::doorExists(const ExpansionRoom* other) const {
  if (!doors.empty()) {
    for (ExpansionDoor* currDoor : doors) {
      if (currDoor->firstRoom == other || currDoor->secondRoom == other) {
        return true;
      }
    }
  }
  return false;
}

bool ObstacleExpansionRoom::createOverlapDoor(ObstacleExpansionRoom* other) {
  if (this->doorExists(other)) {
    return false;
  }

  if (!(this->item->isRoutable() && other->item->isRoutable())) {
    return false;
  }

  if (!this->item->sharesNet(other->item)) {
    return false;
  }

  if (this->item == other->item) {
    PolylineTrace* trace = dynamic_cast<PolylineTrace*>(this->item);
    if (!trace) {
      return false;
    }
    // Create only doors between consecutive trace segments
    if (this->indexInItem != other->indexInItem + 1 &&
        this->indexInItem != other->indexInItem - 1) {
      return false;
    }
  }

  ExpansionDoor* newDoor = new ExpansionDoor(this, other, 2);
  this->addDoor(newDoor);
  other->addDoor(newDoor);
  return true;
}

void ObstacleExpansionRoom::resetDoors() {
  for (ExpandableObject* currDoor : doors) {
    currDoor->reset();
  }
}

bool ObstacleExpansionRoom::removeDoor(ExpandableObject* door) {
  auto it = std::find(doors.begin(), doors.end(), dynamic_cast<ExpansionDoor*>(door));
  if (it != doors.end()) {
    doors.erase(it);
    return true;
  }
  return false;
}

} // namespace freerouting
