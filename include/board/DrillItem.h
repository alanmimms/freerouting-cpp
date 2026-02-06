#ifndef FREEROUTING_BOARD_DRILLITEM_H
#define FREEROUTING_BOARD_DRILLITEM_H

#include "Item.h"
#include "core/Padstack.h"
#include "geometry/Vector2.h"

namespace freerouting {

// Base class for board items that are drilled (Pins and Vias)
// These items have a center point and use a padstack to define their shape
class DrillItem : public Item {
public:
  // Get the center point of this drill item
  IntPoint getCenter() const { return center_; }

  // Set the center point
  void setCenter(IntPoint newCenter) { center_ = newCenter; }

  // Get the padstack defining this drill item's shape
  virtual const Padstack* getPadstack() const = 0;

  // Get first layer (from padstack)
  int firstLayer() const override {
    const Padstack* ps = getPadstack();
    return ps ? ps->fromLayer() : 0;
  }

  // Get last layer (from padstack)
  int lastLayer() const override {
    const Padstack* ps = getPadstack();
    return ps ? ps->toLayer() : 0;
  }

  // Get bounding box (simplified for Phase 4)
  // TODO: Calculate actual shape bounds when full geometry is implemented
  IntBox getBoundingBox() const override {
    // Simple box around center point
    // Real implementation would use padstack shape
    constexpr int kDefaultRadius = 500; // Default radius in board units
    return IntBox(
      center_.x - kDefaultRadius, center_.y - kDefaultRadius,
      center_.x + kDefaultRadius, center_.y + kDefaultRadius
    );
  }

  // Check if this drill item is an obstacle for a trace
  bool isTraceObstacle(int netNumber) const override {
    // Drill items are obstacles for traces of other nets
    return !containsNet(netNumber);
  }

protected:
  // Constructor for derived classes
  DrillItem(IntPoint center, const std::vector<int>& nets,
            int clearanceClass, int id, int componentNumber,
            FixedState fixedState, BasicBoard* board)
    : Item(nets, clearanceClass, id, componentNumber, fixedState, board),
      center_(center) {}

private:
  IntPoint center_;  // Center location of the drill item
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_DRILLITEM_H
