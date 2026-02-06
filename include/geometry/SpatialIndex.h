#ifndef FREEROUTING_GEOMETRY_SPATIALINDEX_H
#define FREEROUTING_GEOMETRY_SPATIALINDEX_H

#include "geometry/IntBox.h"
#include "geometry/Vector2.h"
#include <vector>
#include <map>
#include <algorithm>

namespace freerouting {

// Simple grid-based spatial index for efficient collision detection
// Divides space into cells and stores items by their bounding boxes
// This is a simplified Phase 5 implementation - full R-tree will come later
template<typename ITEM>
class SpatialIndex {
public:
  // Create spatial index with given cell size
  explicit SpatialIndex(int cellSizeValue = 10000)
    : cellSize(cellSizeValue) {
    if (cellSize <= 0) {
      cellSize = 10000;
    }
  }

  // Insert item with its bounding box
  void insert(ITEM* item, const IntBox& bounds) {
    if (!item || bounds.isEmpty()) {
      return;
    }

    // Calculate grid cells overlapped by bounds
    int minCellX = bounds.ll.x / cellSize;
    int minCellY = bounds.ll.y / cellSize;
    int maxCellX = bounds.ur.x / cellSize;
    int maxCellY = bounds.ur.y / cellSize;

    // Add item to all overlapping cells
    for (int cy = minCellY; cy <= maxCellY; ++cy) {
      for (int cx = minCellX; cx <= maxCellX; ++cx) {
        CellKey key{cx, cy};
        cells[key].push_back(item);
      }
    }
  }

  // Remove item from index
  void remove(ITEM* item, const IntBox& bounds) {
    if (!item || bounds.isEmpty()) {
      return;
    }

    int minCellX = bounds.ll.x / cellSize;
    int minCellY = bounds.ll.y / cellSize;
    int maxCellX = bounds.ur.x / cellSize;
    int maxCellY = bounds.ur.y / cellSize;

    // Remove item from all overlapping cells
    for (int cy = minCellY; cy <= maxCellY; ++cy) {
      for (int cx = minCellX; cx <= maxCellX; ++cx) {
        CellKey key{cx, cy};
        auto it = cells.find(key);
        if (it != cells.end()) {
          auto& itemList = it->second;
          itemList.erase(
            std::remove(itemList.begin(), itemList.end(), item),
            itemList.end()
          );

          // Remove empty cells
          if (itemList.empty()) {
            cells.erase(it);
          }
        }
      }
    }
  }

  // Query items in region (returns all items whose bounding boxes overlap region)
  std::vector<ITEM*> query(const IntBox& region) const {
    if (region.isEmpty()) {
      return {};
    }

    int minCellX = region.ll.x / cellSize;
    int minCellY = region.ll.y / cellSize;
    int maxCellX = region.ur.x / cellSize;
    int maxCellY = region.ur.y / cellSize;

    std::vector<ITEM*> result;
    std::vector<ITEM*> seenItems;

    // Collect items from all overlapping cells
    for (int cy = minCellY; cy <= maxCellY; ++cy) {
      for (int cx = minCellX; cx <= maxCellX; ++cx) {
        CellKey key{cx, cy};
        auto it = cells.find(key);
        if (it != cells.end()) {
          for (ITEM* item : it->second) {
            // Avoid duplicates (item may be in multiple cells)
            if (std::find(seenItems.begin(), seenItems.end(), item) == seenItems.end()) {
              result.push_back(item);
              seenItems.push_back(item);
            }
          }
        }
      }
    }

    return result;
  }

  // Query items near a point (within distance)
  std::vector<ITEM*> queryNear(IntPoint point, int distance) const {
    IntBox searchBox(
      point.x - distance, point.y - distance,
      point.x + distance, point.y + distance
    );
    return query(searchBox);
  }

  // Clear all items from index
  void clear() {
    cells.clear();
  }

  // Get number of cells in use
  size_t cellCount() const {
    return cells.size();
  }

  // Get total number of item references (counting duplicates across cells)
  size_t itemReferenceCount() const {
    size_t count = 0;
    for (const auto& [key, items] : cells) {
      count += items.size();
    }
    return count;
  }

private:
  struct CellKey {
    int x;
    int y;

    bool operator<(const CellKey& other) const {
      if (x != other.x) return x < other.x;
      return y < other.y;
    }

    bool operator==(const CellKey& other) const {
      return x == other.x && y == other.y;
    }
  };

  int cellSize;
  std::map<CellKey, std::vector<ITEM*>> cells;
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_SPATIALINDEX_H
