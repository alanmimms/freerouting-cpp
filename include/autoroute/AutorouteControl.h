#ifndef FREEROUTING_AUTOROUTE_AUTOROUTECONTROL_H
#define FREEROUTING_AUTOROUTE_AUTOROUTECONTROL_H

#include <vector>

namespace freerouting {

// Forward declaration
class RoutingBoard;

// Structure for controlling the autoroute algorithm
// Contains all parameters and settings for the routing process
class AutorouteControl {
public:
  // Cost factors for trace expansion in horizontal and vertical directions
  struct ExpansionCostFactor {
    double horizontal;
    double vertical;

    ExpansionCostFactor() : horizontal(1.0), vertical(1.0) {}
    ExpansionCostFactor(double h, double v) : horizontal(h), vertical(v) {}
  };

  // Via costs between layers
  struct ViaCost {
    std::vector<double> costs; // costs[to_layer]

    explicit ViaCost(int layerCount) : costs(layerCount, 0.0) {}
  };

  // Via mask information (which layer ranges a via can connect)
  struct ViaMask {
    int fromLayer;
    int toLayer;
    double radius;

    ViaMask() : fromLayer(0), toLayer(0), radius(0.0) {}
    ViaMask(int from, int to, double r)
      : fromLayer(from), toLayer(to), radius(r) {}
  };

  // The horizontal and vertical trace costs on each layer
  std::vector<ExpansionCostFactor> traceCosts;

  // Defines for each layer if it may be used for routing
  std::vector<bool> layerActive;

  // The currently used trace half widths on each layer
  std::vector<int> traceHalfWidth;

  // The compensated trace half widths (with clearance)
  std::vector<int> compensatedTraceHalfWidth;

  // Via radii for each layer
  std::vector<double> viaRadiusArr;

  // Additional costs for inserting a via between 2 layers
  std::vector<ViaCost> addViaCosts;

  // The currently used clearance class for traces
  int traceClearanceClassNo;

  // True if layer change by inserting vias is allowed
  bool viasAllowed;

  // True if vias may drill to the pad of SMD pins
  bool attachSmdAllowed;

  // The minimum cost value of all normal vias
  double minNormalViaCost;

  // The minimum cost value of all cheap vias
  double minCheapViaCost;

  // Ripup settings
  bool ripupAllowed;
  int ripupCosts;
  int ripupPassNo;

  // Push-and-shove settings
  bool pushAndShoveEnabled;  // Enable push-and-shove routing

  // Maximum iterations for A* search
  int maxIterations;

  // If true, the autoroute algorithm completes after the first drill
  bool isFanout;

  // Normally true if the autorouter contains no fanout pass
  bool removeUnconnectedVias;

  // The currently used net number
  int netNo;

  // The currently used clearance class for vias
  int viaClearanceClass;

  // The array of possible via ranges used by the autorouter
  std::vector<ViaMask> viaInfoArr;

  // The lower bound for the first layer of vias
  int viaLowerBound;

  // The upper bound for the last layer of vias
  int viaUpperBound;

  // Maximum via radius
  double maxViaRadius;

  // The width of the region around changed traces where traces are pulled tight
  int tidyRegionWidth;

  // The pull tight accuracy of traces
  int pullTightAccuracy;

  // The maximum recursion depth for shoving traces
  int maxShoveTraceRecursionDepth;

  // The maximum recursion depth for shoving obstacles
  int maxShoveViaRecursionDepth;

  // The maximum recursion depth for traces springing over obstacles
  int maxSpringOverRecursionDepth;

  // Allow neckdown (trace width reduction near pads)
  bool withNeckdown;

  int layerCount;

  // Creates a new AutorouteControl with default settings
  AutorouteControl(int numLayers)
    : traceCosts(numLayers),
      layerActive(numLayers, true),
      traceHalfWidth(numLayers, 50),
      compensatedTraceHalfWidth(numLayers, 50),
      viaRadiusArr(numLayers, 25.0),
      addViaCosts(numLayers, ViaCost(numLayers)),
      traceClearanceClassNo(0),
      viasAllowed(true),
      attachSmdAllowed(false),
      minNormalViaCost(100.0),
      minCheapViaCost(50.0),
      ripupAllowed(true),   // Enable ripup by default
      ripupCosts(1000),      // Allow ripup up to 10 items (100 * 10)
      ripupPassNo(0),
      pushAndShoveEnabled(true),  // Enable push-and-shove by default
      maxIterations(100000),  // Default 100k iterations
      isFanout(false),
      removeUnconnectedVias(true),
      netNo(-1),
      viaClearanceClass(0),
      viaLowerBound(0),
      viaUpperBound(numLayers - 1),
      maxViaRadius(50.0),
      tidyRegionWidth(100),
      pullTightAccuracy(50),
      maxShoveTraceRecursionDepth(15),
      maxShoveViaRecursionDepth(5),
      maxSpringOverRecursionDepth(10),
      withNeckdown(false),
      layerCount(numLayers) {
    // Initialize default trace costs
    for (int i = 0; i < numLayers; ++i) {
      traceCosts[i] = ExpansionCostFactor(1.0, 1.0);
    }
  }
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_AUTOROUTECONTROL_H
