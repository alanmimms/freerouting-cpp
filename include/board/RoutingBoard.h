#ifndef FREEROUTING_BOARD_ROUTINGBOARD_H
#define FREEROUTING_BOARD_ROUTINGBOARD_H

#include "board/BasicBoard.h"
#include "board/Item.h"
#include "autoroute/IncompleteConnection.h"
#include "geometry/ShapeTree.h"
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <queue>
#include <iostream>

namespace freerouting {

// Extended board with routing capabilities
// Provides infrastructure for autorouting including:
// - Incomplete connection tracking
// - Spatial search trees for routing
// - Route optimization support
//
// This is a simplified Phase 6 version
// Full autoroute engine will be added in later phases
class RoutingBoard : public BasicBoard {
public:
  // Constructor
  RoutingBoard(const LayerStructure& layers, const ClearanceMatrix& clearanceMatrix)
    : BasicBoard(layers, clearanceMatrix),
      shapeTree_(10000) {}  // 10000 unit cell size

  // Get shape tree for spatial queries during routing
  ShapeTree& getShapeTree() { return shapeTree_; }
  const ShapeTree& getShapeTree() const { return shapeTree_; }

  // Add item and update shape tree
  void addItem(std::unique_ptr<Item> item) {
    if (!item) return;

    Item* itemPtr = item.get();
    BasicBoard::addItem(std::move(item));
    shapeTree_.insert(itemPtr);
  }

  // Remove item and update shape tree
  bool removeItem(int itemId) {
    Item* item = getItem(itemId);
    if (!item) return false;

    shapeTree_.remove(item);
    return BasicBoard::removeItem(itemId);
  }

  // Clear all items and shape tree
  void clear() {
    BasicBoard::clear();
    shapeTree_.clear();
    incompleteConnections_.clear();
  }

  // Incomplete connection management

  // Add incomplete connection
  void addIncompleteConnection(const IncompleteConnection& connection) {
    incompleteConnections_.push_back(connection);
  }

  // Get all incomplete connections
  const std::vector<IncompleteConnection>& getIncompleteConnections() const {
    return incompleteConnections_;
  }

  // Get incomplete connections for a specific net
  std::vector<IncompleteConnection> getIncompleteConnectionsForNet(int netNumber) const {
    std::vector<IncompleteConnection> result;
    for (const auto& conn : incompleteConnections_) {
      if (conn.getNetNumber() == netNumber) {
        result.push_back(conn);
      }
    }
    return result;
  }

  // Count incomplete connections
  size_t incompleteConnectionCount() const {
    size_t count = 0;
    for (const auto& conn : incompleteConnections_) {
      if (!conn.isRouted()) {
        ++count;
      }
    }
    return count;
  }

  // Mark a connection as routed
  void markConnectionRouted(Item* fromItem, Item* toItem, int netNumber) {
    for (auto& conn : incompleteConnections_) {
      if (conn.getFromItem() == fromItem &&
          conn.getToItem() == toItem &&
          conn.getNetNumber() == netNumber) {
        conn.setRouted(true);
        break;
      }
    }
  }

  // Clear autoroute info from all items
  void clearAllAutorouteInfo() {
    for (const auto& item : getItems()) {
      item->clearAutorouteInfo();
    }
  }

  // Find incomplete connections by analyzing board state
  // Uses connectivity tracing to only route unconnected items
  void updateIncompleteConnections() {
    incompleteConnections_.clear();

    int totalNets = 0;
    int netsWithMultipleItems = 0;
    int netsNeedingRouting = 0;
    int maxComponents = 0;
    int totalItemsProcessed = 0;

    // For each net, find items that need connections
    if (const Nets* nets = getNets()) {
      for (int netNum = 1; netNum <= nets->maxNetNumber(); ++netNum) {
        const Net* net = nets->getNet(netNum);
        if (!net) continue;

        totalNets++;

        // Get all items on this net
        std::vector<Item*> netItems = getItemsByNet(netNum);
        if (netItems.size() < 2) continue;

        netsWithMultipleItems++;
        totalItemsProcessed += static_cast<int>(netItems.size());

        // Find connected components using connectivity tracing
        std::vector<std::vector<Item*>> components = findConnectedComponents(netItems, netNum);

        if (components.size() > static_cast<size_t>(maxComponents)) {
          maxComponents = static_cast<int>(components.size());
        }

        // If all items are in one component, net is fully connected
        if (components.size() <= 1) continue;

        netsNeedingRouting++;

        // Create connections between disconnected components
        // Use pins (non-routable items) as endpoints to avoid targeting vias/traces
        // that may be optimized away during routing
        for (size_t i = 0; i + 1 < components.size(); ++i) {
          Item* fromItem = findPinInComponent(components[i]);
          Item* toItem = findPinInComponent(components[i + 1]);
          if (fromItem && toItem) {
            IncompleteConnection conn(fromItem, toItem, netNum);
            incompleteConnections_.push_back(conn);
          }
        }
      }
    }

  }

private:
  ShapeTree shapeTree_;  // Spatial index for routing queries
  std::vector<IncompleteConnection> incompleteConnections_;  // Connections to route

  // Find connected components within a set of items on the same net
  // Returns a vector of components, where each component is a vector of connected items
  std::vector<std::vector<Item*>> findConnectedComponents(
      const std::vector<Item*>& netItems, int netNumber) const {

    std::vector<std::vector<Item*>> components;
    std::set<Item*> visited;

    // For each unvisited item, do BFS to find its connected component
    for (Item* startItem : netItems) {
      if (visited.count(startItem)) continue;

      // Start new component
      std::vector<Item*> component;
      std::queue<Item*> queue;
      queue.push(startItem);
      visited.insert(startItem);

      while (!queue.empty()) {
        Item* current = queue.front();
        queue.pop();
        component.push_back(current);

        // Find items physically connected to current
        std::vector<Item*> neighbors = findPhysicallyConnected(current, netItems);
        for (Item* neighbor : neighbors) {
          if (!visited.count(neighbor)) {
            visited.insert(neighbor);
            queue.push(neighbor);
          }
        }
      }

      if (!component.empty()) {
        components.push_back(component);
      }
    }

    return components;
  }

  // Find items physically connected to the given item
  // Two items are physically connected if:
  // - They are traces sharing an endpoint
  // - They are vias/pins at the same location
  std::vector<Item*> findPhysicallyConnected(
      Item* item, const std::vector<Item*>& candidates) const {

    std::vector<Item*> connected;
    if (!item) return connected;

    // Get item's bounding box for spatial queries
    IntBox itemBox = item->getBoundingBox();
    IntPoint itemCenter((itemBox.ll.x + itemBox.ur.x) / 2,
                        (itemBox.ll.y + itemBox.ur.y) / 2);

    for (Item* candidate : candidates) {
      if (candidate == item) continue;

      IntBox candBox = candidate->getBoundingBox();
      IntPoint candCenter((candBox.ll.x + candBox.ur.x) / 2,
                          (candBox.ll.y + candBox.ur.y) / 2);

      // Check if items overlap or are very close (tolerance for connection)
      const int kConnectionTolerance = 100; // 0.01mm tolerance
      int dx = (itemCenter.x > candCenter.x) ? (itemCenter.x - candCenter.x) : (candCenter.x - itemCenter.x);
      int dy = (itemCenter.y > candCenter.y) ? (itemCenter.y - candCenter.y) : (candCenter.y - itemCenter.y);

      if (dx <= kConnectionTolerance && dy <= kConnectionTolerance) {
        // Items are at same location - check layer overlap
        if (item->lastLayer() >= candidate->firstLayer() &&
            item->firstLayer() <= candidate->lastLayer()) {
          connected.push_back(candidate);
        }
      }
    }

    return connected;
  }

  // Find a pin (non-routable item) within a component to use as endpoint
  // Pins are stable endpoints that won't be optimized away during routing
  // Returns nullptr if component is empty or contains only routable items
  Item* findPinInComponent(const std::vector<Item*>& component) const {
    for (Item* item : component) {
      if (item && !item->isRoutable()) {
        return item;  // Found a pin or other fixed endpoint
      }
    }
    // No pin found - this component is only vias/traces (routing artifacts)
    // Do NOT use fallback - these items may be ripped up during routing
    return nullptr;
  }
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_ROUTINGBOARD_H
