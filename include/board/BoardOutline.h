#ifndef FREEROUTING_BOARD_BOARDOUTLINE_H
#define FREEROUTING_BOARD_BOARDOUTLINE_H

#include "board/Item.h"
#include "geometry/IntBox.h"
#include "geometry/Shape.h"
#include "geometry/PolyLine.h"
#include <vector>
#include <memory>

namespace freerouting {

// Represents the physical outline of the PCB board
// Defines the keepout area outside the board boundary
class BoardOutline : public Item {
public:
  // Create board outline from shapes
  BoardOutline(int idNo, int layerCount,
               const std::vector<std::unique_ptr<Shape>>& shapes = {})
    : Item({}, 0, idNo, 0, FixedState::SystemFixed, nullptr),
      layerSpan(layerCount),
      keepoutOutside(true) {
    for (const auto& shape : shapes) {
      addShape(std::make_unique<PolyLine>(*dynamic_cast<const PolyLine*>(shape.get())));
    }
  }

  // Add a shape to the outline
  void addShape(std::unique_ptr<Shape> shape) {
    if (shape) {
      outlineShapes.push_back(std::move(shape));
    }
  }

  // Add polyline shape
  void addPolyLine(const PolyLine& polyline) {
    outlineShapes.push_back(std::make_unique<PolyLine>(polyline));
  }

  // Get outline shapes
  const std::vector<std::unique_ptr<Shape>>& getShapes() const {
    return outlineShapes;
  }

  // Check if point is inside board outline
  bool contains(IntPoint point) const {
    for (const auto& shape : outlineShapes) {
      if (shape->contains(point)) {
        return true;
      }
    }
    return false;
  }

  // Get bounding box of outline
  IntBox getBoundingBox() const override {
    if (outlineShapes.empty()) {
      return IntBox();
    }

    IntBox bbox = outlineShapes[0]->getBoundingBox();
    for (size_t i = 1; i < outlineShapes.size(); ++i) {
      bbox = bbox.unionWith(outlineShapes[i]->getBoundingBox());
    }

    return bbox;
  }

  // Check if item is an obstacle
  bool isObstacle(const Item& other) const override {
    // Board outline is obstacle to all items except other outlines
    (void)other;
    return true;
  }

  // Get first layer (outlines span all layers)
  int firstLayer() const override {
    return 0;
  }

  // Get last layer (outlines span all layers)
  int lastLayer() const override {
    return layerSpan - 1;
  }

  // Copy method
  Item* copy(int newId) const override {
    return new BoardOutline(newId, layerSpan);
  }

  // Check if keepout is outside outline
  bool hasKeepoutOutside() const {
    return keepoutOutside;
  }

private:
  int layerSpan;
  bool keepoutOutside;
  std::vector<std::unique_ptr<Shape>> outlineShapes;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_BOARDOUTLINE_H
