#ifndef FREEROUTING_AUTOROUTE_PATHFINDER_H
#define FREEROUTING_AUTOROUTE_PATHFINDER_H

#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include "board/RoutingBoard.h"
#include <vector>
#include <queue>
#include <map>
#include <cmath>
#include <limits>

namespace freerouting {

// Simple grid-based A* pathfinding for PCB routing
// This is a simplified Phase 6 implementation
// Full routing engine with push-and-shove will come in later phases
class PathFinder {
public:
  // Path search result
  struct PathResult {
    bool found;
    std::vector<IntPoint> points;
    double cost;

    PathResult() : found(false), cost(0.0) {}
  };

  // Create pathfinder with grid resolution
  explicit PathFinder(int gridResolution = 100)
    : gridSize_(gridResolution) {}

  // Find path from start to goal on specific layer
  // Returns empty path if no route found
  PathResult findPath(RoutingBoard& board,
                      IntPoint start, IntPoint goal,
                      int layer, int netNumber);

  // Set grid resolution (smaller = more precise but slower)
  void setGridResolution(int resolution) {
    gridSize_ = resolution;
  }

  // Get grid resolution
  int getGridResolution() const {
    return gridSize_;
  }

private:
  int gridSize_;  // Grid cell size for discretization

  // Grid node for A* search
  struct Node {
    IntPoint point;
    double gCost;  // Cost from start
    double hCost;  // Heuristic to goal
    double fCost;  // Total cost
    Node* parent;

    Node(IntPoint p, double g, double h, Node* par = nullptr)
      : point(p), gCost(g), hCost(h), fCost(g + h), parent(par) {}

    bool operator>(const Node& other) const {
      return fCost > other.fCost;
    }
  };

  // Calculate heuristic (Manhattan distance)
  static double heuristic(IntPoint a, IntPoint b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
  }

  // Snap point to grid
  IntPoint snapToGrid(IntPoint p) const {
    return IntPoint(
      (p.x / gridSize_) * gridSize_,
      (p.y / gridSize_) * gridSize_
    );
  }

  // Check if point is clear for routing
  bool isPointClear(RoutingBoard& board, IntPoint point, int layer, int netNumber) const {
    // Query nearby items
    IntBox searchBox(
      point.x - gridSize_, point.y - gridSize_,
      point.x + gridSize_, point.y + gridSize_
    );

    auto obstacles = board.getShapeTree().findTraceObstacles(
      netNumber, searchBox, layer, layer);

    // Check if any obstacles overlap this point
    for (Item* item : obstacles) {
      IntBox bbox = item->getBoundingBox();
      if (bbox.contains(point)) {
        return false;
      }
    }

    return true;
  }

  // Get neighbors for A* search (4-directional)
  std::vector<IntPoint> getNeighbors(IntPoint point) const {
    return {
      IntPoint(point.x + gridSize_, point.y),
      IntPoint(point.x - gridSize_, point.y),
      IntPoint(point.x, point.y + gridSize_),
      IntPoint(point.x, point.y - gridSize_)
    };
  }
};

// Find path using A* algorithm
inline PathFinder::PathResult PathFinder::findPath(
    RoutingBoard& board,
    IntPoint start, IntPoint goal,
    int layer, int netNumber) {

  PathResult result;

  // Snap to grid
  start = snapToGrid(start);
  goal = snapToGrid(goal);

  // Check if start/goal are clear
  if (!isPointClear(board, start, layer, netNumber)) {
    return result;  // Start blocked
  }
  if (!isPointClear(board, goal, layer, netNumber)) {
    return result;  // Goal blocked
  }

  // Priority queue for open set
  std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;

  // Closed set (visited nodes)
  std::map<IntPoint, double> closedSet;

  // Start node
  Node startNode(start, 0.0, heuristic(start, goal));
  openSet.push(startNode);

  // A* search
  constexpr int kMaxIterations = 10000;  // Prevent infinite loops
  int iterations = 0;

  while (!openSet.empty() && iterations < kMaxIterations) {
    ++iterations;

    // Get node with lowest f-cost
    Node current = openSet.top();
    openSet.pop();

    // Check if reached goal
    if (current.point == goal) {
      // Reconstruct path
      result.found = true;
      result.cost = current.gCost;

      Node* node = &current;
      while (node) {
        result.points.push_back(node->point);
        node = node->parent;
      }

      // Reverse to get start-to-goal order
      std::reverse(result.points.begin(), result.points.end());
      return result;
    }

    // Mark as visited
    closedSet[current.point] = current.gCost;

    // Check neighbors
    for (IntPoint neighbor : getNeighbors(current.point)) {
      // Skip if already visited with better cost
      auto it = closedSet.find(neighbor);
      if (it != closedSet.end() && it->second <= current.gCost + gridSize_) {
        continue;
      }

      // Skip if blocked
      if (!isPointClear(board, neighbor, layer, netNumber)) {
        continue;
      }

      // Add to open set
      double gCost = current.gCost + gridSize_;
      double hCost = heuristic(neighbor, goal);

      // Note: This simplified version doesn't properly track parent pointers
      // A full implementation would use a node pool or smart pointers
      openSet.emplace(neighbor, gCost, hCost, nullptr);
    }
  }

  // No path found
  return result;
}

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_PATHFINDER_H
