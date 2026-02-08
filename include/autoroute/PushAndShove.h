#ifndef FREEROUTING_AUTOROUTE_PUSHANDSHOVE_H
#define FREEROUTING_AUTOROUTE_PUSHANDSHOVE_H

#include "board/Item.h"
#include "board/Trace.h"
#include "board/RoutingBoard.h"
#include "geometry/Vector2.h"
#include <vector>
#include <map>

namespace freerouting {

// Push-and-Shove routing engine
// Allows routing by temporarily moving existing traces out of the way
class PushAndShove {
public:
  // Result of a push-and-shove attempt
  struct PushResult {
    bool success;           // Did we successfully push obstacles?
    int totalCost;          // Total cost of the push operation
    std::vector<Item*> movedItems;    // Items that were moved
    std::vector<Item*> removedItems;  // Items that were removed
  };

  PushAndShove(RoutingBoard* board) : board_(board) {}

  // Try to push obstacles out of the way for a proposed trace
  // Returns true if successful, false if obstacles can't be moved
  PushResult tryPushObstacles(
    IntPoint start,
    IntPoint end,
    int layer,
    int halfWidth,
    int netNo,
    int ripupCostLimit);

  // Calculate the cost of moving an item
  int calculateMoveCost(Item* item, IntVector moveOffset) const;

  // Calculate the cost of removing an item
  int calculateRemoveCost(Item* item) const;

  // Try to reroute a trace segment around an obstacle
  bool tryRerouteTrace(
    Trace* trace,
    IntPoint avoidStart,
    IntPoint avoidEnd,
    std::vector<Item*>& newTraces);

  // Check if a trace can be moved safely
  bool canMoveTrace(Trace* trace, IntVector offset) const;

  // Check if a trace can be removed safely (won't disconnect circuit)
  bool canRemoveTrace(Trace* trace) const;

private:
  RoutingBoard* board_;

  // Find alternative path for a trace segment
  bool findAlternativePath(
    Trace* trace,
    IntBox avoidRegion,
    std::vector<IntPoint>& alternativePath) const;

  // Check if moving/removing item would violate design rules
  bool violatesDesignRules(Item* item, IntVector moveOffset) const;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_PUSHANDSHOVE_H
