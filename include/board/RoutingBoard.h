#ifndef FREEROUTING_BOARD_ROUTINGBOARD_H
#define FREEROUTING_BOARD_ROUTINGBOARD_H

#include "board/BasicBoard.h"
#include "board/Item.h"
#include "autoroute/IncompleteConnection.h"
#include "geometry/ShapeTree.h"
#include <vector>
#include <memory>

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
  // Looks for unconnected items on the same net
  void updateIncompleteConnections() {
    incompleteConnections_.clear();

    // For each net, find items that need connections
    if (const Nets* nets = getNets()) {
      for (int netNum = 1; netNum <= nets->maxNetNumber(); ++netNum) {
        const Net* net = nets->getNet(netNum);
        if (!net) continue;

        // Get all items on this net
        std::vector<Item*> netItems = getItemsByNet(netNum);

        // Simple heuristic: connect each item to nearest unconnected item
        // (Full algorithm would use Steiner tree / minimum spanning tree)
        for (size_t i = 0; i < netItems.size(); ++i) {
          for (size_t j = i + 1; j < netItems.size(); ++j) {
            // Check if items are not already connected
            // (simplified check - full version would trace connectivity)
            IncompleteConnection conn(netItems[i], netItems[j], netNum);
            incompleteConnections_.push_back(conn);
          }
        }
      }
    }
  }

private:
  ShapeTree shapeTree_;  // Spatial index for routing queries
  std::vector<IncompleteConnection> incompleteConnections_;  // Connections to route
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_ROUTINGBOARD_H
