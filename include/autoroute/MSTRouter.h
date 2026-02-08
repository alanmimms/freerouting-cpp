#ifndef FREEROUTING_AUTOROUTE_MSTROUTER_H
#define FREEROUTING_AUTOROUTE_MSTROUTER_H

#include "board/Item.h"
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace freerouting {

// Minimum Spanning Tree router for multi-pad nets
// Uses Prim's algorithm to build optimal connection topology
class MSTRouter {
public:
  // Edge in the MST (connects two items)
  struct MSTEdge {
    Item* from;
    Item* to;
    double cost;  // Airline distance

    MSTEdge(Item* f, Item* t, double c) : from(f), to(t), cost(c) {}
  };

  // Build MST for a set of items (pads/pins) using Prim's algorithm
  // Returns list of edges to route in optimal order
  static std::vector<MSTEdge> buildMST(const std::vector<Item*>& items) {
    std::vector<MSTEdge> edges;

    if (items.size() < 2) {
      return edges;  // No edges needed
    }

    // Track which items are in the MST
    std::vector<bool> inMST(items.size(), false);

    // Start with first item
    inMST[0] = true;
    int itemsInMST = 1;

    // Prim's algorithm: repeatedly add cheapest edge connecting MST to non-MST item
    while (itemsInMST < static_cast<int>(items.size())) {
      double minCost = 1e20;
      int bestFrom = -1;
      int bestTo = -1;

      // Find cheapest edge from MST to non-MST
      for (int i = 0; i < static_cast<int>(items.size()); i++) {
        if (!inMST[i]) continue;

        for (int j = 0; j < static_cast<int>(items.size()); j++) {
          if (inMST[j]) continue;

          double cost = calculateDistance(items[i], items[j]);
          if (cost < minCost) {
            minCost = cost;
            bestFrom = i;
            bestTo = j;
          }
        }
      }

      if (bestFrom >= 0 && bestTo >= 0) {
        // Add edge to MST
        edges.push_back(MSTEdge(items[bestFrom], items[bestTo], minCost));
        inMST[bestTo] = true;
        itemsInMST++;
      } else {
        break;  // No more edges (shouldn't happen)
      }
    }

    return edges;
  }

  // Calculate airline distance between two items
  static double calculateDistance(const Item* a, const Item* b) {
    if (!a || !b) return 1e20;

    // Get bounding boxes
    IntBox boxA = a->getBoundingBox();
    IntBox boxB = b->getBoundingBox();

    // Use box centers for distance calculation
    IntPoint centerA((boxA.ll.x + boxA.ur.x) / 2, (boxA.ll.y + boxA.ur.y) / 2);
    IntPoint centerB((boxB.ll.x + boxB.ur.x) / 2, (boxB.ll.y + boxB.ur.y) / 2);

    int dx = centerB.x - centerA.x;
    int dy = centerB.y - centerA.y;

    return std::sqrt(static_cast<double>(dx * dx + dy * dy));
  }

  // Sort edges by cost (shortest first) for optimal routing order
  static void sortEdgesByCost(std::vector<MSTEdge>& edges) {
    std::sort(edges.begin(), edges.end(),
      [](const MSTEdge& a, const MSTEdge& b) {
        return a.cost < b.cost;
      });
  }
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_MSTROUTER_H
