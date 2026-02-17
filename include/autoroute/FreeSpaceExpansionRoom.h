#ifndef FREEROUTING_AUTOROUTE_FREESPACEEXPANSIONROOM_H
#define FREEROUTING_AUTOROUTE_FREESPACEEXPANSIONROOM_H

#include "autoroute/ExpansionRoom.h"
#include "autoroute/ExpandableObject.h"
#include "geometry/Shape.h"
#include <vector>
#include <algorithm>

namespace freerouting {

// Forward declaration
class ExpansionDoor;

// Expansion areas used by the maze search algorithm
// Represents free space where routing is allowed
class FreeSpaceExpansionRoom : public virtual ExpansionRoom {
public:
  // Creates a new FreeSpaceExpansionRoom
  // The shape is normally unbounded at construction time
  // The final shape will be a subshape that doesn't overlap obstacles
  FreeSpaceExpansionRoom(const Shape* initialShape, int layerNum)
    : shape(initialShape), layer(layerNum) {}

  virtual ~FreeSpaceExpansionRoom() = default;

  // Add door to the list of doors
  void addDoor(ExpansionDoor* door) override {
    doors.push_back(door);
  }

  // Get the list of doors
  std::vector<ExpansionDoor*>& getDoors() override {
    return doors;
  }

  const std::vector<ExpansionDoor*>& getDoors() const override {
    return doors;
  }

  // Remove all doors
  void clearDoors() override {
    doors.clear();
  }

  // Reset doors for next connection
  void resetDoors() override;

  // Remove specific door
  bool removeDoor(ExpandableObject* door) override;

  // Get the shape of this room
  const Shape* getShape() const override {
    return shape;
  }

  // Set the shape of this room
  void setShape(const Shape* newShape) {
    shape = newShape;
  }

  // Get the layer
  int getLayer() const override {
    return layer;
  }

  // Check if door already exists to other room
  bool doorExists(const ExpansionRoom* other) const override;

  // Simplified box-based bounds (Phase 1)
  void setBounds(const IntBox& box) {
    bounds_ = box;
  }

  IntBox getBounds() const {
    return bounds_;
  }

private:
  const Shape* shape;
  int layer;
  std::vector<ExpansionDoor*> doors;
  IntBox bounds_;  // Simplified bounds for Phase 1
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_FREESPACEEXPANSIONROOM_H
