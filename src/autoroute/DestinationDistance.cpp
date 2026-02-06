#include "autoroute/DestinationDistance.h"
#include <cmath>
#include <algorithm>

namespace freerouting {

DestinationDistance::DestinationDistance(
    const std::vector<AutorouteControl::ExpansionCostFactor>& costs,
    const std::vector<bool>& active,
    double normalViaCost,
    double cheapViaCost)
  : traceCosts(costs),
    layerActive(active),
    layerCount(static_cast<int>(active.size())),
    minNormalViaCost(normalViaCost),
    minCheapViaCost(cheapViaCost),
    minComponentSideTraceCost(1.0),
    maxComponentSideTraceCost(1.0),
    minSolderSideTraceCost(1.0),
    maxSolderSideTraceCost(1.0),
    maxInnerSideTraceCost(1.0),
    minComponentInnerTraceCost(1.0),
    minSolderInnerTraceCost(1.0),
    minComponentSolderInnerTraceCost(1.0),
    componentSideBox(),
    solderSideBox(),
    innerSideBox(),
    boxIsEmpty(true),
    componentSideBoxIsEmpty(true),
    solderSideBoxIsEmpty(true),
    innerSideBoxIsEmpty(true) {

  // Count active layers
  activeLayerCount = 0;
  for (int i = 0; i < layerCount; ++i) {
    if (layerActive[i]) {
      ++activeLayerCount;
    }
  }

  // Calculate trace costs for component side (layer 0)
  if (layerCount > 0 && layerActive[0]) {
    minComponentSideTraceCost = std::min(
      traceCosts[0].horizontal, traceCosts[0].vertical);
    maxComponentSideTraceCost = std::max(
      traceCosts[0].horizontal, traceCosts[0].vertical);
  }

  // Calculate trace costs for solder side (last layer)
  if (layerCount > 0 && layerActive[layerCount - 1]) {
    const auto& cost = traceCosts[layerCount - 1];
    minSolderSideTraceCost = std::min(cost.horizontal, cost.vertical);
    maxSolderSideTraceCost = std::max(cost.horizontal, cost.vertical);
  }

  // Calculate maximum inner layer trace cost
  maxInnerSideTraceCost = std::min(
    maxComponentSideTraceCost, maxSolderSideTraceCost);

  for (int i = 1; i < layerCount - 1; ++i) {
    if (!layerActive[i]) continue;

    double maxCost = std::max(
      traceCosts[i].horizontal, traceCosts[i].vertical);
    maxInnerSideTraceCost = std::min(maxInnerSideTraceCost, maxCost);
  }

  minComponentInnerTraceCost = std::min(
    minComponentSideTraceCost, maxInnerSideTraceCost);
  minSolderInnerTraceCost = std::min(
    minSolderSideTraceCost, maxInnerSideTraceCost);
  minComponentSolderInnerTraceCost = std::min(
    minComponentInnerTraceCost, minSolderInnerTraceCost);
}

void DestinationDistance::join(const IntBox& box, int layer) {
  if (layer == 0) {
    componentSideBox = componentSideBoxIsEmpty ?
      box : componentSideBox.unionWith(box);
    componentSideBoxIsEmpty = false;
  } else if (layer == layerCount - 1) {
    solderSideBox = solderSideBoxIsEmpty ?
      box : solderSideBox.unionWith(box);
    solderSideBoxIsEmpty = false;
  } else {
    innerSideBox = innerSideBoxIsEmpty ?
      box : innerSideBox.unionWith(box);
    innerSideBoxIsEmpty = false;
  }
  boxIsEmpty = false;
}

double DestinationDistance::calculate(const IntPoint& point, int layer) const {
  if (boxIsEmpty) {
    return 0.0;
  }

  double result = std::numeric_limits<double>::max();

  // Calculate distance to component side destinations
  if (!componentSideBoxIsEmpty) {
    double dist = calculateDistance(
      point, componentSideBox, minComponentSideTraceCost);

    if (layer != 0) {
      dist += minNormalViaCost; // Add via cost for layer change
    }

    result = std::min(result, dist);
  }

  // Calculate distance to solder side destinations
  if (!solderSideBoxIsEmpty) {
    double dist = calculateDistance(
      point, solderSideBox, minSolderSideTraceCost);

    if (layer != layerCount - 1) {
      dist += minNormalViaCost;
    }

    result = std::min(result, dist);
  }

  // Calculate distance to inner layer destinations
  if (!innerSideBoxIsEmpty && layer > 0 && layer < layerCount - 1) {
    double dist = calculateDistance(
      point, innerSideBox, maxInnerSideTraceCost);
    result = std::min(result, dist);
  }

  return result;
}

void DestinationDistance::reset() {
  componentSideBox = IntBox();
  solderSideBox = IntBox();
  innerSideBox = IntBox();
  boxIsEmpty = true;
  componentSideBoxIsEmpty = true;
  solderSideBoxIsEmpty = true;
  innerSideBoxIsEmpty = true;
}

double DestinationDistance::calculateDistance(
    const IntPoint& from, const IntBox& to, double costFactor) const {

  // Calculate Manhattan distance from point to nearest point in box
  int dx = 0;
  int dy = 0;

  if (from.x < to.ll.x) {
    dx = to.ll.x - from.x;
  } else if (from.x > to.ur.x) {
    dx = from.x - to.ur.x;
  }

  if (from.y < to.ll.y) {
    dy = to.ll.y - from.y;
  } else if (from.y > to.ur.y) {
    dy = from.y - to.ur.y;
  }

  // Apply cost factor (assumes cost in preferred direction)
  return (dx + dy) * costFactor;
}

} // namespace freerouting
