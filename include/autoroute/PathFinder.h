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
    int directionX;  // Direction from parent (-1, 0, 1)
    int directionY;  // Direction from parent (-1, 0, 1)

    Node(IntPoint p, double g, double h, Node* par = nullptr, int dx = 0, int dy = 0)
      : point(p), gCost(g), hCost(h), fCost(g + h), parent(par),
        directionX(dx), directionY(dy) {}

    bool operator>(const Node& other) const {
      return fCost > other.fCost;
    }
  };

  // Calculate heuristic (Octile distance - optimal for 8-directional movement)
  static double heuristic(IntPoint a, IntPoint b) {
    int dx = std::abs(a.x - b.x);
    int dy = std::abs(a.y - b.y);
    // Octile distance: D * (dx + dy) + (D2 - 2 * D) * min(dx, dy)
    // where D = straight cost, D2 = diagonal cost
    // For grid movement: D = 1, D2 = sqrt(2) ≈ 1.414
    constexpr double D = 1.0;
    constexpr double D2 = 1.414;
    return D * (dx + dy) + (D2 - 2.0 * D) * std::min(dx, dy);
  }

  // Snap point to grid
  IntPoint snapToGrid(IntPoint p) const {
    return IntPoint(
      (p.x / gridSize_) * gridSize_,
      (p.y / gridSize_) * gridSize_
    );
  }

  // Check if point is clear for routing (overlap check only)
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

  // Neighbor info for pathfinding
  struct Neighbor {
    IntPoint point;
    int dirX;  // Direction: -1, 0, or 1
    int dirY;
    double cost;  // Movement cost (1.0 for straight, 1.414 for diagonal)
  };

  // Get neighbors for A* search (8-directional with costs)
  std::vector<Neighbor> getNeighbors(IntPoint point) const {
    constexpr double STRAIGHT_COST = 1.0;
    constexpr double DIAGONAL_COST = 1.414;  // sqrt(2)

    return {
      // Straight moves (lower cost)
      {IntPoint(point.x + gridSize_, point.y), 1, 0, STRAIGHT_COST},
      {IntPoint(point.x - gridSize_, point.y), -1, 0, STRAIGHT_COST},
      {IntPoint(point.x, point.y + gridSize_), 0, 1, STRAIGHT_COST},
      {IntPoint(point.x, point.y - gridSize_), 0, -1, STRAIGHT_COST},
      // Diagonal moves (higher cost but more direct)
      {IntPoint(point.x + gridSize_, point.y + gridSize_), 1, 1, DIAGONAL_COST},
      {IntPoint(point.x + gridSize_, point.y - gridSize_), 1, -1, DIAGONAL_COST},
      {IntPoint(point.x - gridSize_, point.y + gridSize_), -1, 1, DIAGONAL_COST},
      {IntPoint(point.x - gridSize_, point.y - gridSize_), -1, -1, DIAGONAL_COST}
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

  // Node storage - keeps nodes alive for parent pointers
  std::map<IntPoint, Node> nodeMap;

  // Priority queue for open set (stores pointers to nodes in nodeMap)
  auto nodePtrCmp = [](const Node* a, const Node* b) {
    return a->fCost > b->fCost;
  };
  std::priority_queue<Node*, std::vector<Node*>, decltype(nodePtrCmp)> openSet(nodePtrCmp);

  // Closed set (visited points)
  std::map<IntPoint, double> closedSet;

  // Create start node
  double startHeuristic = heuristic(start, goal) * gridSize_;
  auto [startIt, _] = nodeMap.emplace(start, Node(start, 0.0, startHeuristic));
  openSet.push(&startIt->second);

  // A* search
  constexpr int kMaxIterations = 100000;  // Increased limit for complex routes
  constexpr double BEND_PENALTY = 0.5;  // Cost penalty for changing direction
  int iterations = 0;

  while (!openSet.empty() && iterations < kMaxIterations) {
    ++iterations;

    // Get node with lowest f-cost
    Node* current = openSet.top();
    openSet.pop();

    // Skip if already processed with better cost
    auto closedIt = closedSet.find(current->point);
    if (closedIt != closedSet.end() && closedIt->second <= current->gCost) {
      continue;
    }

    // Check if reached goal
    if (current->point == goal) {
      // Reconstruct path
      result.found = true;
      result.cost = current->gCost;

      Node* node = current;
      while (node) {
        result.points.push_back(node->point);
        node = node->parent;
      }

      // Reverse to get start-to-goal order
      std::reverse(result.points.begin(), result.points.end());
      return result;
    }

    // Mark as visited
    closedSet[current->point] = current->gCost;

    // Check neighbors
    for (const Neighbor& neighbor : getNeighbors(current->point)) {
      // Skip if already visited with better cost
      auto it = closedSet.find(neighbor.point);
      if (it != closedSet.end()) {
        continue;
      }

      // Skip if blocked
      if (!isPointClear(board, neighbor.point, layer, netNumber)) {
        continue;
      }

      // Calculate movement cost (distance * grid size)
      double moveCost = neighbor.cost * gridSize_;

      // Apply bend penalty if changing direction
      double bendCost = 0.0;
      if (current->parent != nullptr) {
        // Check if direction changed from parent -> current -> neighbor
        bool directionChanged = (current->directionX != neighbor.dirX ||
                                 current->directionY != neighbor.dirY);
        if (directionChanged) {
          bendCost = BEND_PENALTY * gridSize_;
        }
      }

      // Total cost from start
      double gCost = current->gCost + moveCost + bendCost;

      // Check if we already have this node with better cost
      auto existingIt = nodeMap.find(neighbor.point);
      if (existingIt != nodeMap.end() && existingIt->second.gCost <= gCost) {
        continue;
      }

      // Heuristic cost to goal
      double hCost = heuristic(neighbor.point, goal) * gridSize_;

      // Create or update node with parent pointer and direction
      auto [nodeIt, inserted] = nodeMap.emplace(
        neighbor.point,
        Node(neighbor.point, gCost, hCost, current, neighbor.dirX, neighbor.dirY)
      );

      if (!inserted) {
        // Update existing node if new path is better
        nodeIt->second.gCost = gCost;
        nodeIt->second.hCost = hCost;
        nodeIt->second.fCost = gCost + hCost;
        nodeIt->second.parent = current;
        nodeIt->second.directionX = neighbor.dirX;
        nodeIt->second.directionY = neighbor.dirY;
      }

      // Add to open set
      openSet.push(&nodeIt->second);
    }
  }

  // No path found
  return result;
}

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_PATHFINDER_H
