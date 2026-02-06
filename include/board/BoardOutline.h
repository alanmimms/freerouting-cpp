#ifndef FREEROUTING_BOARD_BOARDOUTLINE_H
#define FREEROUTING_BOARD_BOARDOUTLINE_H

#include "board/Item.h"
#include "geometry/IntBox.h"
#include <vector>

namespace freerouting {

// Forward declarations
class Shape;

// Represents the physical outline of the PCB board
// Defines the keepout area outside the board boundary
class BoardOutline : public Item {
public:
  // Create board outline from shapes
  // (Stub - simplified constructor)
  BoardOutline(int idNo, int layerCount)
    : Item({}, 0, idNo, 0, FixedState::SystemFixed, nullptr),
      layerSpan(layerCount),
      keepoutOutside(true) {}

  // Get bounding box of outline
  IntBox getBoundingBox() const override {
    // TODO: Calculate from actual outline shapes
    return IntBox();
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
  // TODO: Add outline shapes when Shape hierarchy is complete
  // std::vector<Shape*> outlineShapes;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_BOARDOUTLINE_H
