#ifndef FREEROUTING_AUTOROUTE_CONNECTION_H
#define FREEROUTING_AUTOROUTE_CONNECTION_H

#include "geometry/Vector2.h"
#include "board/Item.h"
#include "board/Trace.h"
#include <vector>
#include <set>
#include <cmath>
#include <limits>

namespace freerouting {

// Describes a routing connection ending at the next fork or terminal item
// A connection is a continuous path of traces/vias between two fixed points
// Used to track and measure routing quality
class Connection {
public:
  // Detour calculation constants
  static constexpr double kDetourAdd = 100.0;
  static constexpr double kDetourItemCost = 0.1;

  // Create connection with endpoints and items
  Connection(IntPoint startPt, int startLy, IntPoint endPt, int endLy,
             const std::vector<Item*>& items)
    : startPoint(startPt),
      startLayer(startLy),
      endPoint(endPt),
      endLayer(endLy),
      items_(items),
      hasStartPoint(true),
      hasEndPoint(true) {}

  // Create connection with optional endpoints (may end in empty space)
  Connection(bool hasStart, IntPoint startPt, int startLy,
             bool hasEnd, IntPoint endPt, int endLy,
             const std::vector<Item*>& items)
    : startPoint(startPt),
      startLayer(startLy),
      endPoint(endPt),
      endLayer(endLy),
      items_(items),
      hasStartPoint(hasStart),
      hasEndPoint(hasEnd) {}

  // Get connection for an item
  // Traces connection from item until next fork or terminal item
  static Connection* get(Item* item);

  // Get start/end points
  IntPoint getStartPoint() const { return startPoint; }
  IntPoint getEndPoint() const { return endPoint; }

  // Get start/end layers
  int getStartLayer() const { return startLayer; }
  int getEndLayer() const { return endLayer; }

  // Check if connection has valid endpoints
  bool hasStart() const { return hasStartPoint; }
  bool hasEnd() const { return hasEndPoint; }

  // Get items in this connection
  const std::vector<Item*>& getItems() const { return items_; }

  // Get item count
  size_t itemCount() const { return items_.size(); }

  // Calculate total trace length in this connection
  double traceLength() const {
    double result = 0.0;
    for (Item* item : items_) {
      if (auto* trace = dynamic_cast<Trace*>(item)) {
        result += trace->getLength();
      }
    }
    return result;
  }

  // Calculate detour factor (actual length / minimum length)
  // Returns estimate of routing quality (1.0 = optimal, higher = worse)
  double getDetour() const {
    if (!hasStartPoint || !hasEndPoint) {
      return std::numeric_limits<double>::max();
    }

    IntVector delta = endPoint - startPoint;
    double minLength = std::sqrt(static_cast<double>(delta.lengthSquared()));

    double actualLength = traceLength();
    double detour = (actualLength + kDetourAdd) / (minLength + kDetourAdd);
    detour += kDetourItemCost * (items_.size() - 1);

    return detour;
  }

  // Check if connection contains an item
  bool containsItem(const Item* item) const {
    for (const Item* connItem : items_) {
      if (connItem == item) {
        return true;
      }
    }
    return false;
  }

  // Get nets in this connection (from first item)
  std::vector<int> getNets() const {
    if (items_.empty()) {
      return {};
    }
    return items_[0]->getNets();
  }

public:
  IntPoint startPoint;
  int startLayer;
  IntPoint endPoint;
  int endLayer;

private:
  std::vector<Item*> items_;
  bool hasStartPoint;
  bool hasEndPoint;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_CONNECTION_H
