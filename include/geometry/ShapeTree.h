#ifndef FREEROUTING_GEOMETRY_SHAPETREE_H
#define FREEROUTING_GEOMETRY_SHAPETREE_H

#include "geometry/SpatialIndex.h"
#include "board/Item.h"
#include "geometry/IntBox.h"
#include <vector>

namespace freerouting {

// Spatial search structure for board items
// Combines spatial indexing with item-specific collision detection
// This is the Phase 5 integration between geometry and board structures
class ShapeTree {
public:
  // Create shape tree with given cell size
  explicit ShapeTree(int cellSize = 10000)
    : index(cellSize) {}

  // Insert item into the tree
  void insert(Item* item) {
    if (!item) return;
    index.insert(item, item->getBoundingBox());
  }

  // Remove item from the tree
  void remove(Item* item) {
    if (!item) return;
    index.remove(item, item->getBoundingBox());
  }

  // Find all items in region
  std::vector<Item*> queryRegion(const IntBox& region) const {
    return index.query(region);
  }

  // Find all items near a point
  std::vector<Item*> queryNear(IntPoint point, int distance) const {
    return index.queryNear(point, distance);
  }

  // Find all obstacles for a given item
  // An obstacle is any item that:
  // - Is on an overlapping layer
  // - Doesn't share a net with the query item
  // - Has overlapping bounding box
  std::vector<Item*> findObstacles(const Item& queryItem) const {
    std::vector<Item*> candidates = index.query(queryItem.getBoundingBox());
    std::vector<Item*> obstacles;

    for (Item* candidate : candidates) {
      // Skip self
      if (candidate->getId() == queryItem.getId()) {
        continue;
      }

      // Check layer overlap
      if (candidate->lastLayer() < queryItem.firstLayer() ||
          candidate->firstLayer() > queryItem.lastLayer()) {
        continue;
      }

      // Check if it's an obstacle
      if (candidate->isObstacle(queryItem)) {
        obstacles.push_back(candidate);
      }
    }

    return obstacles;
  }

  // Find all obstacles for a trace on a specific net in a region
  std::vector<Item*> findTraceObstacles(int netNumber, const IntBox& region,
                                         int firstLayer, int lastLayer) const {
    std::vector<Item*> candidates = index.query(region);
    std::vector<Item*> obstacles;

    for (Item* candidate : candidates) {
      // Check layer overlap
      if (candidate->lastLayer() < firstLayer ||
          candidate->firstLayer() > lastLayer) {
        continue;
      }

      // Check if it's an obstacle for this net
      if (candidate->isTraceObstacle(netNumber)) {
        obstacles.push_back(candidate);
      }
    }

    return obstacles;
  }

  // Find items on a specific layer in a region
  std::vector<Item*> findItemsOnLayer(int layer, const IntBox& region) const {
    std::vector<Item*> candidates = index.query(region);
    std::vector<Item*> result;

    for (Item* candidate : candidates) {
      if (candidate->isOnLayer(layer)) {
        result.push_back(candidate);
      }
    }

    return result;
  }

  // Find items belonging to a specific net
  std::vector<Item*> findItemsByNet(int netNumber, const IntBox& region) const {
    std::vector<Item*> candidates = index.query(region);
    std::vector<Item*> result;

    for (Item* candidate : candidates) {
      if (candidate->containsNet(netNumber)) {
        result.push_back(candidate);
      }
    }

    return result;
  }

  // Clear all items
  void clear() {
    index.clear();
  }

  // Get statistics
  size_t cellCount() const { return index.cellCount(); }
  size_t itemReferenceCount() const { return index.itemReferenceCount(); }

private:
  SpatialIndex<Item> index;
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_SHAPETREE_H
