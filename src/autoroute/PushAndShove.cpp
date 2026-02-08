#include "autoroute/PushAndShove.h"
#include "board/Via.h"
#include "geometry/IntBox.h"
#include <algorithm>
#include <limits>

namespace freerouting {

PushAndShove::PushResult PushAndShove::tryPushObstacles(
    IntPoint start,
    IntPoint end,
    int layer,
    int halfWidth,
    int netNo,
    int ripupCostLimit) {

  PushResult result;
  result.success = false;
  result.totalCost = 0;

  if (!board_) return result;

  // Create bounding box for the proposed trace
  int minX = std::min(start.x, end.x) - halfWidth;
  int maxX = std::max(start.x, end.x) + halfWidth;
  int minY = std::min(start.y, end.y) - halfWidth;
  int maxY = std::max(start.y, end.y) + halfWidth;
  IntBox traceBox(minX, minY, maxX, maxY);

  // Get clearance requirements
  const ClearanceMatrix& clearance = board_->getClearanceMatrix();
  int requiredClearance = clearance.getValue(1, 1, layer, true);

  // Expand box for clearance query
  IntBox queryBox(
    traceBox.ll.x - requiredClearance,
    traceBox.ll.y - requiredClearance,
    traceBox.ur.x + requiredClearance,
    traceBox.ur.y + requiredClearance
  );

  // Find all items in the conflict region
  const auto& items = board_->getShapeTree().queryRegion(queryBox);

  // Categorize obstacles
  std::vector<Trace*> traceObstacles;
  std::vector<Item*> otherObstacles;

  for (Item* item : items) {
    if (!item) continue;

    // Skip items on different layers
    if (item->lastLayer() < layer || item->firstLayer() > layer) {
      continue;
    }

    // Skip items on the same net
    const auto& itemNets = item->getNets();
    if (std::find(itemNets.begin(), itemNets.end(), netNo) != itemNets.end()) {
      continue;
    }

    // Skip fixed items (can't be moved)
    if (item->isUserFixed()) {
      result.success = false;
      return result;  // Can't push fixed items
    }

    // Check if item actually intersects
    if (queryBox.intersects(item->getBoundingBox())) {
      Trace* trace = dynamic_cast<Trace*>(item);
      if (trace) {
        traceObstacles.push_back(trace);
      } else {
        otherObstacles.push_back(item);
      }
    }
  }

  // For now, we can only handle trace obstacles (not vias/pins)
  if (!otherObstacles.empty()) {
    result.success = false;
    return result;
  }

  // Try to remove or reroute each trace obstacle
  for (Trace* trace : traceObstacles) {
    // First, try to reroute the trace around the obstacle
    std::vector<Item*> reroutedTraces;
    bool canReroute = tryRerouteTrace(trace, start, end, reroutedTraces);

    if (canReroute && !reroutedTraces.empty()) {
      // Rerouting succeeded - calculate cost as trace length difference
      int rerouteCost = calculateRemoveCost(trace);  // Cost of removing original
      result.movedItems.push_back(trace);
      result.totalCost += rerouteCost / 2;  // Reroute is cheaper than removal

      // Mark the new traces for addition
      for (Item* newTrace : reroutedTraces) {
        result.movedItems.push_back(newTrace);
      }
    } else {
      // Rerouting failed - try simple removal
      int removeCost = calculateRemoveCost(trace);

      // Check if removal is within budget
      if (result.totalCost + removeCost <= ripupCostLimit) {
        // Check if trace can be safely removed
        if (canRemoveTrace(trace)) {
          result.removedItems.push_back(trace);
          result.totalCost += removeCost;
        } else {
          // Can't safely remove this trace
          result.success = false;
          return result;
        }
      } else {
        // Can't afford to remove this trace
        result.success = false;
        return result;
      }
    }
  }

  result.success = true;
  return result;
}

int PushAndShove::calculateMoveCost(Item* item, IntVector moveOffset) const {
  if (!item) return INT32_MAX;
  if (item->isUserFixed()) return INT32_MAX;

  // Base cost for moving an item
  int baseCost = 200;

  // Longer moves are more expensive
  int distance = std::sqrt(static_cast<double>(moveOffset.lengthSquared()));
  int distanceCost = distance / 100;  // 1 cost per 100nm moved

  return baseCost + distanceCost;
}

int PushAndShove::calculateRemoveCost(Item* item) const {
  if (!item) return INT32_MAX;
  if (item->isUserFixed()) return INT32_MAX;

  // Traces have lower removal cost (can be rerouted)
  Trace* trace = dynamic_cast<Trace*>(item);
  if (trace) {
    // Cost based on trace length
    double length = trace->getLength();
    return static_cast<int>(50 + length / 1000);  // Base 50 + length/1000nm
  }

  // Vias are more expensive to remove
  Via* via = dynamic_cast<Via*>(item);
  if (via) {
    return 500;  // High cost for via removal
  }

  // Other items (pins, etc) can't be removed
  return INT32_MAX;
}

bool PushAndShove::tryRerouteTrace(
    Trace* trace,
    IntPoint avoidStart,
    IntPoint avoidEnd,
    std::vector<Item*>& newTraces) {

  if (!trace || !board_) return false;

  // Define avoidance region
  int margin = trace->getHalfWidth() * 2;
  int minX = std::min(avoidStart.x, avoidEnd.x) - margin;
  int maxX = std::max(avoidStart.x, avoidEnd.x) + margin;
  int minY = std::min(avoidStart.y, avoidEnd.y) - margin;
  int maxY = std::max(avoidStart.y, avoidEnd.y) + margin;
  IntBox avoidRegion(minX, minY, maxX, maxY);

  // Simple strategy: try to route around the obstacle
  // Full implementation would use A* to find alternative path
  std::vector<IntPoint> alternativePath;
  if (!findAlternativePath(trace, avoidRegion, alternativePath)) {
    return false;  // No alternative path found
  }

  // Create new trace segments for the alternative path
  // (Simplified - full implementation would create actual traces)
  return true;
}

bool PushAndShove::canMoveTrace(Trace* trace, IntVector offset) const {
  if (!trace || !board_) return false;
  if (trace->isUserFixed()) return false;

  // Check if moved position would violate design rules
  return !violatesDesignRules(trace, offset);
}

bool PushAndShove::canRemoveTrace(Trace* trace) const {
  if (!trace || !board_) return false;
  if (trace->isUserFixed()) return false;

  // A trace can be removed if it doesn't disconnect the circuit
  // This requires connectivity analysis which we'll implement later
  // For now, assume all non-fixed traces can be removed
  return true;
}

bool PushAndShove::findAlternativePath(
    Trace* trace,
    IntBox avoidRegion,
    std::vector<IntPoint>& alternativePath) const {

  if (!trace) return false;

  IntPoint start = trace->getStart();
  IntPoint end = trace->getEnd();

  // Simple heuristic: try to route around the obstacle
  // If trace is horizontal, try routing above/below
  // If trace is vertical, try routing left/right

  if (trace->isHorizontal()) {
    // Try routing above
    IntPoint mid1(start.x, avoidRegion.ur.y + trace->getHalfWidth() * 2);
    IntPoint mid2(end.x, avoidRegion.ur.y + trace->getHalfWidth() * 2);
    alternativePath = {start, mid1, mid2, end};
    return true;
  } else if (trace->isVertical()) {
    // Try routing to the right
    IntPoint mid1(avoidRegion.ur.x + trace->getHalfWidth() * 2, start.y);
    IntPoint mid2(avoidRegion.ur.x + trace->getHalfWidth() * 2, end.y);
    alternativePath = {start, mid1, mid2, end};
    return true;
  }

  // For diagonal traces, use simplified approach
  return false;
}

bool PushAndShove::violatesDesignRules(Item* item, IntVector moveOffset) const {
  if (!item || !board_) return true;

  // Get the moved bounding box
  IntBox currentBox = item->getBoundingBox();
  IntBox movedBox(
    currentBox.ll.x + moveOffset.x,
    currentBox.ll.y + moveOffset.y,
    currentBox.ur.x + moveOffset.x,
    currentBox.ur.y + moveOffset.y
  );

  // Check if moved box would overlap with fixed items
  const auto& items = board_->getShapeTree().queryRegion(movedBox);
  for (Item* other : items) {
    if (!other || other == item) continue;
    if (other->isUserFixed() && movedBox.intersects(other->getBoundingBox())) {
      return true;  // Would violate clearance with fixed item
    }
  }

  return false;
}

} // namespace freerouting
