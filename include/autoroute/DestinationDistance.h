#ifndef FREEROUTING_AUTOROUTE_DESTINATIONDISTANCE_H
#define FREEROUTING_AUTOROUTE_DESTINATIONDISTANCE_H

#include "autoroute/AutorouteControl.h"
#include "geometry/IntBox.h"
#include "geometry/Vector2.h"
#include <vector>
#include <limits>

namespace freerouting {

// Calculation of a good lower bound for the distance between a new
// MazeExpansionElement and the destination set of the expansion.
class DestinationDistance {
public:
  // Creates a new instance of DestinationDistance.
  // traceCosts and layerActive are arrays of dimension layerCount.
  DestinationDistance(
    const std::vector<AutorouteControl::ExpansionCostFactor>& traceCosts,
    const std::vector<bool>& layerActive,
    double minNormalViaCost,
    double minCheapViaCost);

  // Add a destination box on a specific layer
  void join(const IntBox& box, int layer);

  // Calculate distance from a point on a layer
  double calculate(const FloatPoint& point, int layer);

  // Calculate distance from an integer point on a layer
  double calculate(const IntPoint& point, int layer);

  // Calculate distance from a box on a layer
  double calculate(const IntBox& box, int layer);

  // Calculate cheap distance (using cheap via costs)
  double calculateCheapDistance(const IntBox& box, int layer);

private:
  const std::vector<AutorouteControl::ExpansionCostFactor>& traceCosts;
  const std::vector<bool>& layerActive;
  const int layerCount;
  const int activeLayerCount;
  const double minCheapViaCost;

  double minComponentSideTraceCost;
  double maxComponentSideTraceCost;
  double minSolderSideTraceCost;
  double maxSolderSideTraceCost;
  double maxInnerSideTraceCost;
  // minimum of the maximal trace costs on each inner layer
  double minComponentInnerTraceCost;
  // minimum of minComponentSideTraceCost and maxInnerSideTraceCost
  double minSolderInnerTraceCost;
  // minimum of minSolderSideTraceCost and maxInnerSideTraceCost
  double minComponentSolderInnerTraceCost;
  // minimum of minComponentInnerTraceCost and minSolderInnerTraceCost
  double minNormalViaCost;

  IntBox componentSideBox;
  IntBox solderSideBox;
  IntBox innerSideBox;

  bool boxIsEmpty;
  bool componentSideBoxIsEmpty;
  bool solderSideBoxIsEmpty;
  bool innerSideBoxIsEmpty;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_DESTINATIONDISTANCE_H
