#include "autoroute/LeeRouter.h"
#include "board/RoutingBoard.h"
#include <queue>
#include <algorithm>
#include <cmath>

namespace freerouting {

void LeeRouter::worldToGrid(IntPoint world, const GridInfo& info, int& gx, int& gy) {
  gx = (world.x - info.origin.x) / info.gridSize;
  gy = (world.y - info.origin.y) / info.gridSize;
}

IntPoint LeeRouter::gridToWorld(int gx, int gy, const GridInfo& info) {
  return IntPoint(
    info.origin.x + gx * info.gridSize + info.gridSize / 2,
    info.origin.y + gy * info.gridSize + info.gridSize / 2
  );
}

void LeeRouter::markObstacles(
    std::vector<GridCell>& grid,
    const GridInfo& info,
    int layer,
    int expansionCells,
    RoutingBoard* board,
    int netNo) {

  // Optimization: Sample grid sparsely (every 4th cell) to speed up obstacle detection
  // The grid is already coarse (4× trace width), so sampling is acceptable
  const int sampleStep = 4;

  for (int gy = 0; gy < info.height; gy += sampleStep) {
    for (int gx = 0; gx < info.width; gx += sampleStep) {
      IntPoint worldPos = gridToWorld(gx, gy, info);

      // Check if traces are prohibited at this location
      if (board->isTraceProhibited(worldPos, layer, netNo)) {
        // Mark this cell and a small region around it
        for (int dy = 0; dy < sampleStep && gy + dy < info.height; ++dy) {
          for (int dx = 0; dx < sampleStep && gx + dx < info.width; ++dx) {
            int idx = layer * info.width * info.height + (gy + dy) * info.width + (gx + dx);
            grid[idx].obstacle = true;
          }
        }
      }
    }
  }
}

bool LeeRouter::expandWave(
    std::vector<GridCell>& grid,
    const GridInfo& info,
    IntPoint start, int startLayer,
    IntPoint goal, int goalLayer,
    int viaCostPenalty) {

  // Convert start and goal to grid coordinates
  int startGx, startGy, goalGx, goalGy;
  worldToGrid(start, info, startGx, startGy);
  worldToGrid(goal, info, goalGx, goalGy);

  // Priority queue for A* search (faster than pure BFS)
  struct WavePoint {
    int gx, gy, layer;
    uint32_t cost;
    uint32_t heuristic;

    uint32_t priority() const { return cost + heuristic; }

    bool operator>(const WavePoint& other) const {
      return priority() > other.priority();
    }
  };

  std::priority_queue<WavePoint, std::vector<WavePoint>, std::greater<WavePoint>> waveQueue;

  // Manhattan heuristic for A*
  auto manhattanDist = [](int gx1, int gy1, int gx2, int gy2) -> uint32_t {
    return static_cast<uint32_t>(std::abs(gx2 - gx1) + std::abs(gy2 - gy1));
  };

  // Initialize start point
  int startIdx = startLayer * info.width * info.height + startGy * info.width + startGx;
  grid[startIdx].cost = 0;
  uint32_t startHeur = manhattanDist(startGx, startGy, goalGx, goalGy);
  waveQueue.push({startGx, startGy, startLayer, 0, startHeur});

  // 4-direction connectivity (Manhattan routing)
  const int dx[] = {1, -1, 0, 0};
  const int dy[] = {0, 0, 1, -1};

  uint32_t goalCost = UINT32_MAX;
  constexpr uint32_t kMaxIterations = 100000;  // Reduced limit
  uint32_t iterations = 0;

  while (!waveQueue.empty() && iterations < kMaxIterations) {
    ++iterations;

    WavePoint current = waveQueue.top();
    waveQueue.pop();

    int currentIdx = current.layer * info.width * info.height +
                     current.gy * info.width + current.gx;

    // Skip if we've found a better path to this cell
    if (grid[currentIdx].cost < current.cost) {
      continue;
    }

    // Check if reached goal
    if (current.gx == goalGx && current.gy == goalGy && current.layer == goalLayer) {
      goalCost = current.cost;
      break;  // Found path
    }

    // Early termination if we've exceeded goal cost
    if (current.cost > goalCost) {
      continue;
    }

    // Expand in 4 directions (same layer)
    for (int dir = 0; dir < 4; ++dir) {
      int ngx = current.gx + dx[dir];
      int ngy = current.gy + dy[dir];

      if (ngx < 0 || ngx >= info.width || ngy < 0 || ngy >= info.height) {
        continue;
      }

      int neighborIdx = current.layer * info.width * info.height +
                       ngy * info.width + ngx;

      // Skip obstacles
      if (grid[neighborIdx].obstacle) {
        continue;
      }

      uint32_t newCost = current.cost + 1;

      // Update if found better path
      if (newCost < grid[neighborIdx].cost) {
        grid[neighborIdx].cost = newCost;
        grid[neighborIdx].dx = -dx[dir];
        grid[neighborIdx].dy = -dy[dir];
        grid[neighborIdx].dz = 0;
        uint32_t heur = manhattanDist(ngx, ngy, goalGx, goalGy);
        waveQueue.push({ngx, ngy, current.layer, newCost, heur});
      }
    }

    // Try layer changes (vias)
    for (int newLayer = 0; newLayer < info.layers; ++newLayer) {
      if (newLayer == current.layer) continue;

      int neighborIdx = newLayer * info.width * info.height +
                       current.gy * info.width + current.gx;

      // Skip obstacles
      if (grid[neighborIdx].obstacle) {
        continue;
      }

      uint32_t newCost = current.cost + viaCostPenalty;

      // Update if found better path
      if (newCost < grid[neighborIdx].cost) {
        grid[neighborIdx].cost = newCost;
        grid[neighborIdx].dx = 0;
        grid[neighborIdx].dy = 0;
        grid[neighborIdx].dz = current.layer - newLayer;
        uint32_t heur = manhattanDist(current.gx, current.gy, goalGx, goalGy);
        waveQueue.push({current.gx, current.gy, newLayer, newCost, heur});
      }
    }
  }

  return goalCost != UINT32_MAX;
}

void LeeRouter::backtrackPath(
    const std::vector<GridCell>& grid,
    const GridInfo& info,
    IntPoint goal, int goalLayer,
    Result& result) {

  int gx, gy;
  worldToGrid(goal, info, gx, gy);
  int layer = goalLayer;

  constexpr int kMaxPathLength = 10000;
  int pathLength = 0;

  while (pathLength < kMaxPathLength) {
    ++pathLength;

    // Add current point to path
    IntPoint worldPos = gridToWorld(gx, gy, info);
    result.pathPoints.push_back(worldPos);
    result.pathLayers.push_back(layer);

    // Get backtrack direction
    int idx = layer * info.width * info.height + gy * info.width + gx;
    const GridCell& cell = grid[idx];

    // Check if reached start (cost = 0)
    if (cell.cost == 0) {
      break;
    }

    // Move in backtrack direction
    gx += cell.dx;
    gy += cell.dy;
    layer += cell.dz;

    // Bounds check
    if (gx < 0 || gx >= info.width || gy < 0 || gy >= info.height ||
        layer < 0 || layer >= info.layers) {
      break;
    }
  }

  // Reverse to go from start to goal
  std::reverse(result.pathPoints.begin(), result.pathPoints.end());
  std::reverse(result.pathLayers.begin(), result.pathLayers.end());
}

LeeRouter::Result LeeRouter::findPath(
    IntPoint start, int startLayer,
    IntPoint goal, int goalLayer,
    int traceHalfWidth,
    int clearance,
    RoutingBoard* board,
    int netNo) {

  Result result;

  if (!board || traceHalfWidth <= 0) {
    return result;
  }

  // Use trace-width-relative grid (4x trace width for good resolution)
  const int gridSize = traceHalfWidth * 4;

  // Calculate clearance expansion in grid cells
  const int expansionCells = (clearance + gridSize - 1) / gridSize;

  // Calculate bounding box from start and goal
  // Add margin for routing flexibility
  const int margin = gridSize * 50;

  IntBox bbox;
  bbox.ll.x = std::min(start.x, goal.x) - margin;
  bbox.ll.y = std::min(start.y, goal.y) - margin;
  bbox.ur.x = std::max(start.x, goal.x) + margin;
  bbox.ur.y = std::max(start.y, goal.y) + margin;

  // Setup grid info
  GridInfo info;
  info.origin = bbox.ll;
  info.gridSize = gridSize;
  info.width = (bbox.ur.x - bbox.ll.x) / gridSize + 1;
  info.height = (bbox.ur.y - bbox.ll.y) / gridSize + 1;
  info.layers = board->getLayers().count();

  // Limit grid size to prevent memory explosion
  constexpr int kMaxGridDim = 2000;
  if (info.width > kMaxGridDim || info.height > kMaxGridDim) {
    // Board too large for current grid - use coarser grid
    const int scaleFactor = std::max(
      (info.width + kMaxGridDim - 1) / kMaxGridDim,
      (info.height + kMaxGridDim - 1) / kMaxGridDim
    );
    info.gridSize *= scaleFactor;
    info.width = (bbox.ur.x - bbox.ll.x) / info.gridSize + 1;
    info.height = (bbox.ur.y - bbox.ll.y) / info.gridSize + 1;
  }

  // Allocate grid
  const size_t gridCells = static_cast<size_t>(info.width) *
                           static_cast<size_t>(info.height) *
                           static_cast<size_t>(info.layers);

  std::vector<GridCell> grid(gridCells);

  // Mark obstacles on all layers
  // TEMPORARILY DISABLED for performance testing
  // TODO: Optimize obstacle marking - current approach is too slow
  // for (int layer = 0; layer < info.layers; ++layer) {
  //   markObstacles(grid, info, layer, expansionCells, board, netNo);
  // }

  // Ensure start and goal cells are not marked as obstacles
  int startGx, startGy, goalGx, goalGy;
  worldToGrid(start, info, startGx, startGy);
  worldToGrid(goal, info, goalGx, goalGy);

  int startIdx = startLayer * info.width * info.height + startGy * info.width + startGx;
  int goalIdx = goalLayer * info.width * info.height + goalGy * info.width + goalGx;

  grid[startIdx].obstacle = false;
  grid[goalIdx].obstacle = false;

  // Via cost penalty (prefer staying on same layer)
  const int viaCostPenalty = 50;

  // Expand wave from start to goal
  bool found = expandWave(grid, info, start, startLayer, goal, goalLayer, viaCostPenalty);

  if (found) {
    result.found = true;
    backtrackPath(grid, info, goal, goalLayer, result);
  }

  return result;
}

} // namespace freerouting
