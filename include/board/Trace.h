#ifndef FREEROUTING_BOARD_TRACE_H
#define FREEROUTING_BOARD_TRACE_H

#include "Item.h"
#include <vector>

namespace freerouting {

// Trace segment on a PCB
// Traces connect pins and vias, forming the electrical connections
//
// NOTE: Phase 4 simplified version with just line segments
// Full polyline support with multiple corners will be added in later phases
class Trace : public Item {
public:
  // Create a new trace segment
  Trace(IntPoint start, IntPoint end, int layer, int halfWidth,
        const std::vector<int>& nets, int clearanceClass, int id,
        FixedState fixedState, BasicBoard* board)
    : Item(nets, clearanceClass, id, 0 /* no component */, fixedState, board),
      start_(start),
      end_(end),
      layer_(layer),
      halfWidth_(halfWidth) {}

  // Get start point
  IntPoint getStart() const { return start_; }

  // Get end point
  IntPoint getEnd() const { return end_; }

  // Get layer
  int getLayer() const { return layer_; }

  // Get half-width (trace width / 2)
  int getHalfWidth() const { return halfWidth_; }

  // Get full width
  int getWidth() const { return halfWidth_ * 2; }

  // Set half-width
  void setHalfWidth(int halfWidth) { halfWidth_ = halfWidth; }

  // Get first/last layer (traces are single-layer in Phase 4)
  int firstLayer() const override { return layer_; }
  int lastLayer() const override { return layer_; }

  // Traces are routable
  bool isRoutable() const override {
    return !isUserFixed() && netCount() > 0;
  }

  // Get bounding box
  IntBox getBoundingBox() const override {
    int minX = std::min(start_.x, end_.x) - halfWidth_;
    int maxX = std::max(start_.x, end_.x) + halfWidth_;
    int minY = std::min(start_.y, end_.y) - halfWidth_;
    int maxY = std::max(start_.y, end_.y) + halfWidth_;
    return IntBox(minX, minY, maxX, maxY);
  }

  // Get length of trace
  double getLength() const {
    IntVector delta = end_ - start_;
    return std::sqrt(static_cast<double>(delta.lengthSquared()));
  }

  // Check if this trace shares a net with another item (convenience method)
  bool sharesNetWith(const Item& other) const {
    return sharesNet(other);
  }

  // Check if this trace is an obstacle for another item
  bool isObstacle(const Item& other) const override {
    // Same trace is not an obstacle
    if (&other == this) return false;

    // Traces of the same net are not obstacles to each other
    // (though spacing rules may apply)
    if (sharesNet(other)) return false;

    // Different net = obstacle
    return true;
  }

  // Check if trace is horizontal
  bool isHorizontal() const {
    return start_.y == end_.y;
  }

  // Check if trace is vertical
  bool isVertical() const {
    return start_.x == end_.x;
  }

  // Check if trace is orthogonal (horizontal or vertical)
  bool isOrthogonal() const {
    return isHorizontal() || isVertical();
  }

  // Check if trace is diagonal (45 degrees)
  bool isDiagonal() const {
    IntVector delta = end_ - start_;
    return std::abs(delta.x) == std::abs(delta.y);
  }

  // Create a copy with new ID
  Item* copy(int newId) const override {
    return new Trace(start_, end_, layer_, halfWidth_, getNets(),
                     getClearanceClass(), newId, getFixedState(), getBoard());
  }

private:
  IntPoint start_;     // Start point
  IntPoint end_;       // End point
  int layer_;          // Layer number
  int halfWidth_;      // Half-width of trace
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_TRACE_H
