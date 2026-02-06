#ifndef FREEROUTING_BOARD_CONDUCTIONAREA_H
#define FREEROUTING_BOARD_CONDUCTIONAREA_H

#include "board/Item.h"
#include "geometry/IntBox.h"
#include "geometry/Shape.h"
#include "geometry/ComplexPolygon.h"
#include <vector>
#include <string>
#include <memory>

namespace freerouting {

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

  // Set the area shape
  void setShape(std::unique_ptr<Shape> shape) {
    areaShape = std::move(shape);
  }

  // Set area shape from polygon
  void setPolygon(const ComplexPolygon& polygon) {
    areaShape = std::make_unique<ComplexPolygon>(polygon);
  }

  // Get area shape
  const Shape* getShape() const {
    return areaShape.get();
  }

  // Check if point is inside this area
  bool contains(IntPoint point) const {
    if (!areaShape) {
      return false;
    }
    return areaShape->contains(point);
  }

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
    if (!areaShape) {
      return IntBox();
    }
    return areaShape->getBoundingBox();
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
  std::unique_ptr<Shape> areaShape;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_CONDUCTIONAREA_H
