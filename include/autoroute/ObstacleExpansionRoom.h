#ifndef FREEROUTING_AUTOROUTE_OBSTACLEEXPANSIONROOM_H
#define FREEROUTING_AUTOROUTE_OBSTACLEEXPANSIONROOM_H

#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "geometry/Shape.h"
#include <vector>

namespace freerouting {

// Forward declarations
class Item;
class ShapeSearchTree;

// Expansion Room used for pushing and ripping obstacles in the autoroute algorithm
// Represents board items (traces, vias) that can potentially be moved or removed
class ObstacleExpansionRoom : public CompleteExpansionRoom {
public:
  // Creates a new ObstacleExpansionRoom
  ObstacleExpansionRoom(Item* obstacleItem, int indexInItem, ShapeSearchTree* shapeTree);

  ~ObstacleExpansionRoom() override = default;

  // Get the layer of this room
  int getLayer() const override;

  // Get the shape of this room
  const Shape* getShape() const override {
    return roomShape;
  }

  // Check if this room has already a 1-dimensional door to other room
  bool doorExists(const ExpansionRoom* other) const override;

  // Add a door to the door list of this room
  void addDoor(ExpansionDoor* door) override {
    doors.push_back(door);
  }

  // Creates a 2-dim door with the other obstacle room
  // Returns false if no door was created
  // Assumes this room and other have a 2-dimensional overlap
  bool createOverlapDoor(ObstacleExpansionRoom* other);

  // Returns the list of doors of this room to neighbour expansion rooms
  std::vector<ExpansionDoor*>& getDoors() override {
    return doors;
  }

  const std::vector<ExpansionDoor*>& getDoors() const override {
    return doors;
  }

  // Removes all doors from this room
  void clearDoors() override {
    doors.clear();
  }

  // Reset all doors
  void resetDoors() override;

  // Get target doors (always empty for obstacle rooms)
  std::vector<TargetItemExpansionDoor*>& getTargetDoors() override {
    return emptyTargetDoors;
  }

  const std::vector<TargetItemExpansionDoor*>& getTargetDoors() const override {
    return emptyTargetDoors;
  }

  // Get the item this room represents
  Item* getItem() const {
    return item;
  }

  // Get the index in the item
  int getIndexInItem() const {
    return indexInItem;
  }

  // Remove door from this room
  bool removeDoor(ExpandableObject* door) override;

  // Returns if all doors to the neighbour rooms are calculated
  bool allDoorsCalculated() const {
    return doorsCalculated;
  }

  void setDoorsCalculated(bool value) {
    doorsCalculated = value;
  }

private:
  Item* item;
  int indexInItem;
  const Shape* roomShape;
  std::vector<ExpansionDoor*> doors;
  std::vector<TargetItemExpansionDoor*> emptyTargetDoors;  // Always empty
  bool doorsCalculated;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_OBSTACLEEXPANSIONROOM_H
