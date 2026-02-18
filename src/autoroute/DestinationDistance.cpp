#include "autoroute/DestinationDistance.h"
#include <cmath>
#include <algorithm>

namespace freerouting {

DestinationDistance::DestinationDistance(
    const std::vector<AutorouteControl::ExpansionCostFactor>& traceCosts,
    const std::vector<bool>& layerActive,
    double minNormalViaCost,
    double minCheapViaCost)
  : traceCosts(traceCosts),
    layerActive(layerActive),
    layerCount(static_cast<int>(layerActive.size())),
    activeLayerCount([&layerActive]() {
      int count = 0;
      for (int i = 0; i < static_cast<int>(layerActive.size()); ++i) {
        if (layerActive[i]) {
          ++count;
        }
      }
      return count;
    }()),
    minCheapViaCost(minCheapViaCost),
    minComponentSideTraceCost(0.0),
    maxComponentSideTraceCost(0.0),
    minSolderSideTraceCost(0.0),
    maxSolderSideTraceCost(0.0),
    maxInnerSideTraceCost(0.0),
    minComponentInnerTraceCost(0.0),
    minSolderInnerTraceCost(0.0),
    minComponentSolderInnerTraceCost(0.0),
    minNormalViaCost(minNormalViaCost),
    componentSideBox(IntBox::empty()),
    solderSideBox(IntBox::empty()),
    innerSideBox(IntBox::empty()),
    boxIsEmpty(true),
    componentSideBoxIsEmpty(true),
    solderSideBoxIsEmpty(true),
    innerSideBoxIsEmpty(true) {

  if (layerActive[0]) {
    if (traceCosts[0].horizontal < traceCosts[0].vertical) {
      minComponentSideTraceCost = traceCosts[0].horizontal;
      maxComponentSideTraceCost = traceCosts[0].vertical;
    } else {
      minComponentSideTraceCost = traceCosts[0].vertical;
      maxComponentSideTraceCost = traceCosts[0].horizontal;
    }
  }

  if (layerActive[layerCount - 1]) {
    const AutorouteControl::ExpansionCostFactor& currTraceCost = traceCosts[layerCount - 1];

    if (currTraceCost.horizontal < currTraceCost.vertical) {
      minSolderSideTraceCost = currTraceCost.horizontal;
      maxSolderSideTraceCost = currTraceCost.vertical;
    } else {
      minSolderSideTraceCost = currTraceCost.vertical;
      maxSolderSideTraceCost = currTraceCost.horizontal;
    }
  }

  // Note: for inner layers we assume that cost in preferred direction is 1
  maxInnerSideTraceCost = std::min(maxComponentSideTraceCost, maxSolderSideTraceCost);
  for (int ind2 = 1; ind2 < layerCount - 1; ++ind2) {
    if (!layerActive[ind2]) {
      continue;
    }
    double currMaxCost = std::max(traceCosts[ind2].horizontal, traceCosts[ind2].vertical);

    maxInnerSideTraceCost = std::min(maxInnerSideTraceCost, currMaxCost);
  }
  minComponentInnerTraceCost = std::min(minComponentSideTraceCost, maxInnerSideTraceCost);
  minSolderInnerTraceCost = std::min(minSolderSideTraceCost, maxInnerSideTraceCost);
  minComponentSolderInnerTraceCost = std::min(minComponentInnerTraceCost, minSolderInnerTraceCost);
}

void DestinationDistance::join(const IntBox& box, int layer) {
  if (layer == 0) {
    componentSideBox = componentSideBox.unionWith(box);
    componentSideBoxIsEmpty = false;
  } else if (layer == layerCount - 1) {
    solderSideBox = solderSideBox.unionWith(box);
    solderSideBoxIsEmpty = false;
  } else {
    innerSideBox = innerSideBox.unionWith(box);
    innerSideBoxIsEmpty = false;
  }
  boxIsEmpty = false;
}

double DestinationDistance::calculate(const FloatPoint& point, int layer) {
  return calculate(IntBox::fromPoint(IntPoint(static_cast<int>(point.x), static_cast<int>(point.y))), layer);
}

double DestinationDistance::calculate(const IntPoint& point, int layer) {
  return calculate(IntBox::fromPoint(point), layer);
}

double DestinationDistance::calculate(const IntBox& box, int layer) {
  if (boxIsEmpty) {
    return std::numeric_limits<int>::max();
  }

  double componentSideDeltaX;
  double componentSideDeltaY;

  if (box.ll.x > componentSideBox.ur.x) {
    componentSideDeltaX = box.ll.x - componentSideBox.ur.x;
  } else if (box.ur.x < componentSideBox.ll.x) {
    componentSideDeltaX = componentSideBox.ll.x - box.ur.x;
  } else {
    componentSideDeltaX = 0;
  }

  if (box.ll.y > componentSideBox.ur.y) {
    componentSideDeltaY = box.ll.y - componentSideBox.ur.y;
  } else if (box.ur.y < componentSideBox.ll.y) {
    componentSideDeltaY = componentSideBox.ll.y - box.ur.y;
  } else {
    componentSideDeltaY = 0;
  }

  double solderSideDeltaX;
  double solderSideDeltaY;

  if (box.ll.x > solderSideBox.ur.x) {
    solderSideDeltaX = box.ll.x - solderSideBox.ur.x;
  } else if (box.ur.x < solderSideBox.ll.x) {
    solderSideDeltaX = solderSideBox.ll.x - box.ur.x;
  } else {
    solderSideDeltaX = 0;
  }

  if (box.ll.y > solderSideBox.ur.y) {
    solderSideDeltaY = box.ll.y - solderSideBox.ur.y;
  } else if (box.ur.y < solderSideBox.ll.y) {
    solderSideDeltaY = solderSideBox.ll.y - box.ur.y;
  } else {
    solderSideDeltaY = 0;
  }

  double innerSideDeltaX;
  double innerSideDeltaY;

  if (box.ll.x > innerSideBox.ur.x) {
    innerSideDeltaX = box.ll.x - innerSideBox.ur.x;
  } else if (box.ur.x < innerSideBox.ll.x) {
    innerSideDeltaX = innerSideBox.ll.x - box.ur.x;
  } else {
    innerSideDeltaX = 0;
  }

  if (box.ll.y > innerSideBox.ur.y) {
    innerSideDeltaY = box.ll.y - innerSideBox.ur.y;
  } else if (box.ur.y < innerSideBox.ll.y) {
    innerSideDeltaY = innerSideBox.ll.y - box.ur.y;
  } else {
    innerSideDeltaY = 0;
  }

  double componentSideMaxDelta;
  double componentSideMinDelta;

  if (componentSideDeltaX > componentSideDeltaY) {
    componentSideMaxDelta = componentSideDeltaX;
    componentSideMinDelta = componentSideDeltaY;
  } else {
    componentSideMaxDelta = componentSideDeltaY;
    componentSideMinDelta = componentSideDeltaX;
  }

  double solderSideMaxDelta;
  double solderSideMinDelta;

  if (solderSideDeltaX > solderSideDeltaY) {
    solderSideMaxDelta = solderSideDeltaX;
    solderSideMinDelta = solderSideDeltaY;
  } else {
    solderSideMaxDelta = solderSideDeltaY;
    solderSideMinDelta = solderSideDeltaX;
  }

  double innerSideMaxDelta;
  double innerSideMinDelta;

  if (innerSideDeltaX > innerSideDeltaY) {
    innerSideMaxDelta = innerSideDeltaX;
    innerSideMinDelta = innerSideDeltaY;
  } else {
    innerSideMaxDelta = innerSideDeltaY;
    innerSideMinDelta = innerSideDeltaX;
  }

  double result = std::numeric_limits<int>::max();

  if (layer == 0)
  // calculate shortest distance to component side box
  {
    // calculate one layer distance

    if (!componentSideBoxIsEmpty) {
      result = box.weightedDistance(componentSideBox, traceCosts[0].horizontal, traceCosts[0].vertical);
    }

    if (activeLayerCount <= 1) {
      return result;
    }

    // calculate two layer distance on component and solder side

    double tmpDistance;
    if (minSolderSideTraceCost < minComponentSideTraceCost) {
      tmpDistance = minSolderSideTraceCost * solderSideMaxDelta + minComponentSideTraceCost * solderSideMinDelta + minNormalViaCost;
    } else {
      tmpDistance = minComponentSideTraceCost * solderSideMaxDelta + minSolderSideTraceCost * solderSideMinDelta + minNormalViaCost;
    }

    result = std::min(result, tmpDistance);

    // calculate two layer distance on component and solder side
    // with two vias

    tmpDistance = componentSideMaxDelta + componentSideMinDelta * minComponentInnerTraceCost + 2 * minNormalViaCost;

    result = std::min(result, tmpDistance);

    if (activeLayerCount == 2) {
      return result;
    }

    // calculate two layer distance on component side and an inner side

    tmpDistance = innerSideMaxDelta + innerSideMinDelta * minComponentInnerTraceCost + minNormalViaCost;

    result = std::min(result, tmpDistance);

    // calculate three layer distance

    tmpDistance = solderSideMaxDelta + +minComponentSolderInnerTraceCost * solderSideMinDelta + 2 * minNormalViaCost;
    result = std::min(result, tmpDistance);

    tmpDistance = componentSideMaxDelta + componentSideMinDelta + 2 * minNormalViaCost;
    result = std::min(result, tmpDistance);

    if (activeLayerCount == 3) {
      return result;
    }

    tmpDistance = innerSideMaxDelta + innerSideMinDelta + 2 * minNormalViaCost;

    result = std::min(result, tmpDistance);

    // calculate four layer distance

    tmpDistance = solderSideMaxDelta + solderSideMinDelta + 3 * minNormalViaCost;

    result = std::min(result, tmpDistance);

    return result;
  }
  if (layer == layerCount - 1)
  // calculate the shortest distance to solder side box
  {
    // calculate one layer distance

    if (!solderSideBoxIsEmpty) {
      result = box.weightedDistance(solderSideBox, traceCosts[layer].horizontal, traceCosts[layer].vertical);
    }

    // calculate two layer distance
    double tmpDistance;
    if (minComponentSideTraceCost < minSolderSideTraceCost) {
      tmpDistance = minComponentSideTraceCost * componentSideMaxDelta + minSolderSideTraceCost * componentSideMinDelta + minNormalViaCost;
    } else {
      tmpDistance = minSolderSideTraceCost * componentSideMaxDelta + minComponentSideTraceCost * componentSideMinDelta + minNormalViaCost;
    }
    result = std::min(result, tmpDistance);
    tmpDistance = solderSideMaxDelta + solderSideMinDelta * minSolderInnerTraceCost + 2 * minNormalViaCost;
    result = std::min(result, tmpDistance);
    if (activeLayerCount <= 2) {
      return result;
    }
    tmpDistance = innerSideMinDelta * minSolderInnerTraceCost + innerSideMaxDelta + minNormalViaCost;
    result = std::min(result, tmpDistance);

    // calculate three layer distance

    tmpDistance = componentSideMaxDelta + minComponentSolderInnerTraceCost * componentSideMinDelta + 2 * minNormalViaCost;
    result = std::min(result, tmpDistance);
    tmpDistance = solderSideMaxDelta + solderSideMinDelta + 2 * minNormalViaCost;
    result = std::min(result, tmpDistance);
    if (activeLayerCount == 3) {
      return result;
    }
    tmpDistance = innerSideMaxDelta + innerSideMinDelta + 2 * minNormalViaCost;
    result = std::min(result, tmpDistance);

    // calculate four layer distance

    tmpDistance = componentSideMaxDelta + componentSideMinDelta + 3 * minNormalViaCost;
    result = std::min(result, tmpDistance);
    return result;
  }

  // calculate distance to inner layer box

  // calculate one layer distance

  if (!innerSideBoxIsEmpty) {
    result = box.weightedDistance(innerSideBox, traceCosts[layer].horizontal, traceCosts[layer].vertical);
  }

  // calculate two layer distance

  double tmpDistance = innerSideMaxDelta + innerSideMinDelta + minNormalViaCost;

  result = std::min(result, tmpDistance);
  tmpDistance = componentSideMaxDelta + componentSideMinDelta * minComponentInnerTraceCost + minNormalViaCost;
  result = std::min(result, tmpDistance);
  tmpDistance = solderSideMaxDelta + solderSideMinDelta * minSolderInnerTraceCost + minNormalViaCost;
  result = std::min(result, tmpDistance);

  // calculate three layer distance

  tmpDistance = componentSideMaxDelta + componentSideMinDelta + 2 * minNormalViaCost;
  result = std::min(result, tmpDistance);
  tmpDistance = solderSideMaxDelta + solderSideMinDelta + 2 * minNormalViaCost;
  result = std::min(result, tmpDistance);

  return result;
}

double DestinationDistance::calculateCheapDistance(const IntBox& box, int layer) {
  double minNormalViaCostSave = minNormalViaCost;

  minNormalViaCost = minCheapViaCost;
  double result = calculate(box, layer);

  minNormalViaCost = minNormalViaCostSave;
  return result;
}

} // namespace freerouting
