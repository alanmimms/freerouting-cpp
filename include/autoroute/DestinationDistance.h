#ifndef FREEROUTING_AUTOROUTE_DESTINATIONDISTANCE_H
#define FREEROUTING_AUTOROUTE_DESTINATIONDISTANCE_H

#include "autoroute/AutorouteControl.h"
#include "geometry/IntBox.h"
#include <vector>

namespace freerouting {

// Calculation of a good lower bound for the distance between a new maze expansion
// element and the destination set of the expansion
// Uses heuristic distance calculations to guide the search
class DestinationDistance {
public:
  // Creates a new DestinationDistance calculator
  DestinationDistance(
    const std::vector<AutorouteControl::ExpansionCostFactor>& traceCosts,
    const std::vector<bool>& layerActive,
    double minNormalViaCost,
    double minCheapViaCost);

  // Add a destination box on a specific layer
  void join(const IntBox& box, int layer);

  // Calculate the lower bound distance from a point on a layer to destinations
  double calculate(const IntPoint& point, int layer) const;

  // Reset the destination boxes
  void reset();

private:
  const std::vector<AutorouteControl::ExpansionCostFactor>& traceCosts;
  const std::vector<bool>& layerActive;
  int layerCount;
  int activeLayerCount;
  double minNormalViaCost;
  double minCheapViaCost;

  // Cost factors for different layer types
  double minComponentSideTraceCost;
  double maxComponentSideTraceCost;
  double minSolderSideTraceCost;
  double maxSolderSideTraceCost;
  double maxInnerSideTraceCost;
  double minComponentInnerTraceCost;
  double minSolderInnerTraceCost;
  double minComponentSolderInnerTraceCost;

  // Bounding boxes of destination areas
  IntBox componentSideBox;
  IntBox solderSideBox;
  IntBox innerSideBox;

  // Flags indicating if boxes are empty
  bool boxIsEmpty;
  bool componentSideBoxIsEmpty;
  bool solderSideBoxIsEmpty;
  bool innerSideBoxIsEmpty;

  // Helper to calculate distance with cost factor
  double calculateDistance(const IntPoint& from, const IntBox& to, double costFactor) const;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_DESTINATIONDISTANCE_H
