#ifndef FREEROUTING_AUTOROUTE_LEEROUTER_H
#define FREEROUTING_AUTOROUTE_LEEROUTER_H

#include "geometry/Vector2.h"
#include <vector>
#include <cstdint>

namespace freerouting {

class RoutingBoard;

// Lee's algorithm / Maze router with obstacle avoidance
// Classic wave expansion pathfinding optimized for PCB routing
class LeeRouter {
public:
  struct Result {
    bool found;
    std::vector<IntPoint> pathPoints;
    std::vector<int> pathLayers;

    Result() : found(false) {}
  };

  // Find a path from start to goal
  // Uses trace-width-relative grid for performance
  static Result findPath(
    IntPoint start, int startLayer,
    IntPoint goal, int goalLayer,
    int traceHalfWidth,
    int clearance,
    RoutingBoard* board,
    int netNo);

private:
  struct GridCell {
    uint32_t cost;        // Wave front cost (distance from start)
    int16_t dx;           // Backtrack direction x
    int16_t dy;           // Backtrack direction y
    int8_t dz;            // Backtrack direction z (layer change)
    bool obstacle;        // Cell is blocked by obstacle

    GridCell() : cost(UINT32_MAX), dx(0), dy(0), dz(0), obstacle(false) {}
  };

  // Grid dimensions and resolution
  struct GridInfo {
    IntPoint origin;      // World space origin
    int gridSize;         // Grid cell size in internal units
    int width;            // Grid width in cells
    int height;           // Grid height in cells
    int layers;           // Number of layers
  };

  // Convert world coordinates to grid coordinates
  static void worldToGrid(IntPoint world, const GridInfo& info, int& gx, int& gy);

  // Convert grid coordinates to world coordinates (cell center)
  static IntPoint gridToWorld(int gx, int gy, const GridInfo& info);

  // Mark obstacles in grid with clearance expansion
  static void markObstacles(
    std::vector<GridCell>& grid,
    const GridInfo& info,
    int layer,
    int expansionCells,
    RoutingBoard* board,
    int netNo);

  // Wave expansion from start point
  static bool expandWave(
    std::vector<GridCell>& grid,
    const GridInfo& info,
    IntPoint start, int startLayer,
    IntPoint goal, int goalLayer,
    int viaCostPenalty);

  // Backtrack from goal to reconstruct path
  static void backtrackPath(
    const std::vector<GridCell>& grid,
    const GridInfo& info,
    IntPoint goal, int goalLayer,
    Result& result);
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_LEEROUTER_H
