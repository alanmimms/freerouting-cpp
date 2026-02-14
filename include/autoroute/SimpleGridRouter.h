#ifndef FREEROUTING_AUTOROUTE_SIMPLEGRIDROUTER_H
#define FREEROUTING_AUTOROUTE_SIMPLEGRIDROUTER_H

#include "geometry/Vector2.h"
#include <vector>

namespace freerouting {

class RoutingBoard;

// Simple grid-based A* pathfinding router
// This is a temporary/fallback solution until the full expansion room algorithm is ported
class SimpleGridRouter {
public:
  struct Result {
    bool found;
    std::vector<IntPoint> pathPoints;
    std::vector<int> pathLayers;

    Result() : found(false) {}
  };

  // Find a path from start to goal
  static Result findPath(
    IntPoint start, int startLayer,
    IntPoint goal, int goalLayer,
    int traceHalfWidth,
    RoutingBoard* board,
    int netNo);
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_SIMPLEGRIDROUTER_H
