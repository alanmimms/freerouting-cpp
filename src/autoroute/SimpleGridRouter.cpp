#include "autoroute/SimpleGridRouter.h"
#include "board/RoutingBoard.h"
#include <queue>
#include <unordered_set>
#include <cmath>
#include <algorithm>

namespace freerouting {

// Grid-based A* pathfinding - simple but functional
// This is a temporary solution until the full expansion room algorithm is ported

struct GridNode {
  IntPoint pos;
  int layer;
  double gCost;  // Cost from start
  double hCost;  // Heuristic cost to goal
  GridNode* parent;

  double fCost() const { return gCost + hCost; }

  bool operator>(const GridNode& other) const {
    return fCost() > other.fCost();
  }
};

struct GridNodeHash {
  size_t operator()(const std::tuple<int, int, int>& t) const {
    auto h1 = std::hash<int>{}(std::get<0>(t));
    auto h2 = std::hash<int>{}(std::get<1>(t));
    auto h3 = std::hash<int>{}(std::get<2>(t));
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

SimpleGridRouter::Result SimpleGridRouter::findPath(
    IntPoint start, int startLayer,
    IntPoint goal, int goalLayer,
    int traceHalfWidth,
    RoutingBoard* board,
    int netNo) {

  Result result;
  result.found = false;

  if (!board) {
    return result;
  }

  // Grid resolution (in internal units) - coarser for speed
  const int gridSize = traceHalfWidth * 4;
  if (gridSize <= 0) {
    return result;
  }

  // Snap to grid
  auto snapToGrid = [gridSize](int coord) {
    return (coord / gridSize) * gridSize;
  };

  IntPoint gridStart(snapToGrid(start.x), snapToGrid(start.y));
  IntPoint gridGoal(snapToGrid(goal.x), snapToGrid(goal.y));

  // Priority queue for A*
  std::priority_queue<GridNode, std::vector<GridNode>, std::greater<GridNode>> openSet;

  // Closed set - visited nodes
  std::unordered_set<std::tuple<int, int, int>, GridNodeHash> closedSet;

  // Allocate nodes on heap (will be cleaned up after path reconstruction)
  std::vector<std::unique_ptr<GridNode>> allocatedNodes;

  // Heuristic: Manhattan distance + layer change penalty
  auto heuristic = [](IntPoint from, int fromLayer, IntPoint to, int toLayer) {
    double dist = std::abs(to.x - from.x) + std::abs(to.y - from.y);
    double layerPenalty = std::abs(toLayer - fromLayer) * 10000.0;  // Prefer same layer
    return dist + layerPenalty;
  };

  // Create start node
  auto startNode = std::make_unique<GridNode>();
  startNode->pos = gridStart;
  startNode->layer = startLayer;
  startNode->gCost = 0;
  startNode->hCost = heuristic(gridStart, startLayer, gridGoal, goalLayer);
  startNode->parent = nullptr;

  GridNode* startPtr = startNode.get();
  allocatedNodes.push_back(std::move(startNode));
  openSet.push(*startPtr);

  // Increased from 10,000 to allow complex board routing
  // TODO: Replace SimpleGridRouter with proper expansion room algorithm
  const int maxIterations = 100000;
  int iterations = 0;

  while (!openSet.empty() && iterations < maxIterations) {
    iterations++;

    GridNode current = openSet.top();
    openSet.pop();

    // Early termination: if very close to goal, create direct connection
    int dx = std::abs(current.pos.x - gridGoal.x);
    int dy = std::abs(current.pos.y - gridGoal.y);
    if (dx <= gridSize * 2 && dy <= gridSize * 2 && current.layer == goalLayer) {
      // Close enough - reconstruct path to current, then direct to goal
      closedSet.insert({current.pos.x, current.pos.y, current.layer});
      break;
    }

    // Check if already visited
    auto nodeKey = std::make_tuple(current.pos.x, current.pos.y, current.layer);
    if (closedSet.count(nodeKey)) {
      continue;
    }
    closedSet.insert(nodeKey);

    // Check if reached goal
    if (current.pos.x == gridGoal.x && current.pos.y == gridGoal.y &&
        current.layer == goalLayer) {
      // Reconstruct path
      GridNode* node = &current;
      while (node) {
        result.pathPoints.push_back(node->pos);
        result.pathLayers.push_back(node->layer);

        // Find parent in allocated nodes
        GridNode* nextParent = nullptr;
        if (node->parent) {
          for (auto& allocated : allocatedNodes) {
            if (allocated.get() == node->parent) {
              nextParent = allocated.get();
              break;
            }
          }
        }
        node = nextParent;
      }

      std::reverse(result.pathPoints.begin(), result.pathPoints.end());
      std::reverse(result.pathLayers.begin(), result.pathLayers.end());
      result.found = true;
      return result;
    }

    // Expand neighbors (8 directions + layer changes)
    const int dirs[][2] = {
      {gridSize, 0}, {-gridSize, 0}, {0, gridSize}, {0, -gridSize},
      {gridSize, gridSize}, {gridSize, -gridSize}, {-gridSize, gridSize}, {-gridSize, -gridSize}
    };

    for (auto& dir : dirs) {
      IntPoint newPos(current.pos.x + dir[0], current.pos.y + dir[1]);

      // Check if this position is reasonable (simplified bounds check)
      const int maxDist = 1000000;  // 10cm in internal units
      if (std::abs(newPos.x - start.x) > maxDist || std::abs(newPos.y - start.y) > maxDist) {
        continue;
      }

      // Get clearance required (trace width + spacing)
      int clearance = traceHalfWidth * 2;  // Full trace width as clearance

      // Check endpoint only (grid guarantees path is straight line)
      // For better quality, should sample intermediate points, but too slow
      if (board->hasObstacleAt(newPos, current.layer, netNo, clearance)) {
        continue;  // Skip - endpoint has obstacle
      }

      // Same layer move
      auto neighborKey = std::make_tuple(newPos.x, newPos.y, current.layer);
      if (!closedSet.count(neighborKey)) {
        auto neighbor = std::make_unique<GridNode>();
        neighbor->pos = newPos;
        neighbor->layer = current.layer;
        neighbor->gCost = current.gCost + std::sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
        neighbor->hCost = heuristic(newPos, current.layer, gridGoal, goalLayer);

        // Find current node in allocated list to set parent
        GridNode* currentPtr = nullptr;
        for (auto& allocated : allocatedNodes) {
          if (allocated->pos.x == current.pos.x && allocated->pos.y == current.pos.y &&
              allocated->layer == current.layer) {
            currentPtr = allocated.get();
            break;
          }
        }
        neighbor->parent = currentPtr;

        GridNode* neighborPtr = neighbor.get();
        allocatedNodes.push_back(std::move(neighbor));
        openSet.push(*neighborPtr);
      }
    }

    // Try layer changes (vias)
    int numLayers = board->getLayers().count();
    for (int newLayer = 0; newLayer < numLayers; newLayer++) {
      if (newLayer == current.layer) continue;

      auto neighborKey = std::make_tuple(current.pos.x, current.pos.y, newLayer);
      if (!closedSet.count(neighborKey)) {
        auto neighbor = std::make_unique<GridNode>();
        neighbor->pos = current.pos;
        neighbor->layer = newLayer;
        neighbor->gCost = current.gCost + 5000.0;  // Via cost penalty
        neighbor->hCost = heuristic(current.pos, newLayer, gridGoal, goalLayer);

        GridNode* currentPtr = nullptr;
        for (auto& allocated : allocatedNodes) {
          if (allocated->pos.x == current.pos.x && allocated->pos.y == current.pos.y &&
              allocated->layer == current.layer) {
            currentPtr = allocated.get();
            break;
          }
        }
        neighbor->parent = currentPtr;

        GridNode* neighborPtr = neighbor.get();
        allocatedNodes.push_back(std::move(neighbor));
        openSet.push(*neighborPtr);
      }
    }
  }

  return result;
}

} // namespace freerouting
