#ifndef FREEROUTING_BOARD_CONDUCTIONAREA_H
#define FREEROUTING_BOARD_CONDUCTIONAREA_H

#include "board/Item.h"
#include "geometry/IntBox.h"
#include <vector>
#include <string>

namespace freerouting {

// Forward declarations
class Shape;

// Represents a conductive area (copper pour, plane) on the board
// Can be electrically connected to traces and vias
class ConductionArea : public Item {
public:
  // Create conduction area
  ConductionArea(int idNo,
                 int layerNo,
                 const std::vector<int>& nets,
                 const std::string& areaName,
                 bool isObstacleToOtherNets)
    : Item(nets, 0, idNo, 0, FixedState::SystemFixed, nullptr),
      layer(layerNo),
      name(areaName),
      isObstacleFlag(isObstacleToOtherNets) {}

  // Get area name
  const std::string& getName() const { return name; }

  // Check if this area is an obstacle to other items
  bool isObstacle(const Item& other) const override {
    if (!isObstacleFlag) {
      return false;  // Not an obstacle if disabled
    }

    // Obstacle to items on different nets
    if (!sharesNet(other)) {
      return true;
    }

    return false;
  }

  // Get bounding box
  IntBox getBoundingBox() const override {
    // TODO: Calculate from actual area shape
    return IntBox();
  }

  // Get first layer
  int firstLayer() const override {
    return layer;
  }

  // Get last layer
  int lastLayer() const override {
    return layer;
  }

  // Copy method
  Item* copy(int newId) const override {
    return new ConductionArea(newId, layer, getNets(), name, isObstacleFlag);
  }

  // Get connected items
  // (Stub - requires spatial search)
  std::vector<Item*> getConnectedItems() const {
    // TODO: Search for overlapping items on same net
    return {};
  }

  // Check if area acts as obstacle
  bool getIsObstacle() const {
    return isObstacleFlag;
  }

  // Set obstacle flag
  void setIsObstacle(bool value) {
    isObstacleFlag = value;
  }

private:
  int layer;
  std::string name;
  bool isObstacleFlag;
  // TODO: Add area shape when Shape hierarchy is complete
  // Shape* areaShape;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_CONDUCTIONAREA_H
