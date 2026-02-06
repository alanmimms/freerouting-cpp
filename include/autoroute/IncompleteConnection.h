#ifndef FREEROUTING_AUTOROUTE_INCOMPLETECONNECTION_H
#define FREEROUTING_AUTOROUTE_INCOMPLETECONNECTION_H

#include "geometry/Vector2.h"
#include "board/Item.h"
#include <vector>

namespace freerouting {

// Represents an incomplete connection that needs routing
// Tracks two items that should be connected by routing
class IncompleteConnection {
public:
  // Create incomplete connection between two items
  IncompleteConnection(Item* fromItem, Item* toItem, int netNumber)
    : fromItem_(fromItem),
      toItem_(toItem),
      netNumber_(netNumber),
      isRouted_(false) {}

  // Get from/to items
  Item* getFromItem() const { return fromItem_; }
  Item* getToItem() const { return toItem_; }

  // Get net number
  int getNetNumber() const { return netNumber_; }

  // Check if connection is routed
  bool isRouted() const { return isRouted_; }

  // Mark as routed/unrouted
  void setRouted(bool routed) { isRouted_ = routed; }

  // Calculate air-wire distance (direct distance between items)
  double getAirWireDistance() const;

  // Get estimated routing difficulty (0-1, 1 = hardest)
  double getDifficulty() const {
    // Simple heuristic: longer distance = harder
    double distance = getAirWireDistance();
    // Normalize to reasonable range (assume 10000 units is "hard")
    return std::min(distance / 10000.0, 1.0);
  }

  // Compare for sorting (shorter connections first)
  bool operator<(const IncompleteConnection& other) const {
    return getAirWireDistance() < other.getAirWireDistance();
  }

private:
  Item* fromItem_;
  Item* toItem_;
  int netNumber_;
  bool isRouted_;
};

// Calculate air-wire distance
inline double IncompleteConnection::getAirWireDistance() const {
  if (!fromItem_ || !toItem_) {
    return 0.0;
  }

  // Get bounding box centers as approximation
  IntBox fromBox = fromItem_->getBoundingBox();
  IntBox toBox = toItem_->getBoundingBox();

  IntPoint fromCenter(
    (fromBox.ll.x + fromBox.ur.x) / 2,
    (fromBox.ll.y + fromBox.ur.y) / 2
  );

  IntPoint toCenter(
    (toBox.ll.x + toBox.ur.x) / 2,
    (toBox.ll.y + toBox.ur.y) / 2
  );

  IntVector delta = toCenter - fromCenter;
  return std::sqrt(static_cast<double>(delta.lengthSquared()));
}

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_INCOMPLETECONNECTION_H
