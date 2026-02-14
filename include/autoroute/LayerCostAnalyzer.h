#ifndef FREEROUTING_AUTOROUTE_LAYERCOSTANALYZER_H
#define FREEROUTING_AUTOROUTE_LAYERCOSTANALYZER_H

#include "board/RoutingBoard.h"
#include "geometry/IntBox.h"
#include <vector>

namespace freerouting {

// Analyzes routing cost/congestion on each layer
// Used to intelligently select which layer to route on
class LayerCostAnalyzer {
public:
  explicit LayerCostAnalyzer(RoutingBoard* board) : board_(board) {
    if (board_) {
      numLayers_ = board_->getLayers().count();
      layerCosts_.resize(numLayers_, 0.0);
      layerObstacleCounts_.resize(numLayers_, 0);
      layerTraceLength_.resize(numLayers_, 0.0);
    }
  }

  // Analyze all layers and compute routing costs
  void analyze() {
    if (!board_) return;

    // Reset counters
    for (int i = 0; i < numLayers_; ++i) {
      layerCosts_[i] = 0.0;
      layerObstacleCounts_[i] = 0;
      layerTraceLength_[i] = 0.0;
    }

    // Analyze all items on the board
    for (const auto& item : board_->getItems()) {
      if (!item) continue;

      int firstLayer = item->firstLayer();
      int lastLayer = item->lastLayer();

      // Count obstacles and trace length per layer
      for (int layer = firstLayer; layer <= lastLayer; ++layer) {
        if (layer < 0 || layer >= numLayers_) continue;

        layerObstacleCounts_[layer]++;

        // If it's a trace, add its length
        if (const Trace* trace = dynamic_cast<const Trace*>(item.get())) {
          if (trace->getLayer() == layer) {
            layerTraceLength_[layer] += trace->getLength();
          }
        }
      }
    }

    // Calculate board area
    IntBox boardBounds = calculateBoardBounds();
    double boardArea = static_cast<double>(boardBounds.ur.x - boardBounds.ll.x) *
                       static_cast<double>(boardBounds.ur.y - boardBounds.ll.y);
    if (boardArea < 1.0) boardArea = 1.0;

    // Compute normalized costs (0.0 = empty, 1.0+ = congested)
    for (int layer = 0; layer < numLayers_; ++layer) {
      // Cost is combination of obstacle density and trace length density
      double obstacleDensity = layerObstacleCounts_[layer] / 100.0;  // Normalize by typical count
      double traceDensity = layerTraceLength_[layer] / boardArea;

      layerCosts_[layer] = obstacleDensity + traceDensity * 0.5;
    }
  }

  // Get routing cost for a specific layer (lower is better)
  double getLayerCost(int layer) const {
    if (layer < 0 || layer >= numLayers_) return 999999.0;
    return layerCosts_[layer];
  }

  // Get obstacle count on a layer
  int getObstacleCount(int layer) const {
    if (layer < 0 || layer >= numLayers_) return 999999;
    return layerObstacleCounts_[layer];
  }

  // Get total trace length on a layer
  double getTraceLength(int layer) const {
    if (layer < 0 || layer >= numLayers_) return 999999.0;
    return layerTraceLength_[layer];
  }

  // Find the least congested layer (best for routing)
  int findBestLayer() const {
    int bestLayer = 0;
    double minCost = layerCosts_[0];

    for (int layer = 1; layer < numLayers_; ++layer) {
      if (layerCosts_[layer] < minCost) {
        minCost = layerCosts_[layer];
        bestLayer = layer;
      }
    }

    return bestLayer;
  }

  // Find best layer in a specific region of the board
  int findBestLayerInRegion(IntBox region) const {
    if (!board_) return 0;

    // Count obstacles in region for each layer
    std::vector<int> regionalObstacles(numLayers_, 0);

    for (const auto& item : board_->getItems()) {
      if (!item) continue;

      IntBox itemBox = item->getBoundingBox();

      // Check if item overlaps region
      if (itemBox.ur.x < region.ll.x || itemBox.ll.x > region.ur.x ||
          itemBox.ur.y < region.ll.y || itemBox.ll.y > region.ur.y) {
        continue;  // No overlap
      }

      // Count this item for its layers
      for (int layer = item->firstLayer(); layer <= item->lastLayer(); ++layer) {
        if (layer >= 0 && layer < numLayers_) {
          regionalObstacles[layer]++;
        }
      }
    }

    // Find layer with fewest obstacles in this region
    int bestLayer = 0;
    int minObstacles = regionalObstacles[0];

    for (int layer = 1; layer < numLayers_; ++layer) {
      if (regionalObstacles[layer] < minObstacles) {
        minObstacles = regionalObstacles[layer];
        bestLayer = layer;
      }
    }

    return bestLayer;
  }

  // Get layer utilization percentage (0-100+)
  double getLayerUtilization(int layer) const {
    if (layer < 0 || layer >= numLayers_) return 0.0;
    return layerCosts_[layer] * 100.0;
  }

private:
  RoutingBoard* board_;
  int numLayers_ = 0;
  std::vector<double> layerCosts_;           // Overall cost per layer
  std::vector<int> layerObstacleCounts_;     // Number of items per layer
  std::vector<double> layerTraceLength_;     // Total trace length per layer

  IntBox calculateBoardBounds() const {
    IntBox bounds(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);

    for (const auto& item : board_->getItems()) {
      if (!item) continue;
      IntBox itemBox = item->getBoundingBox();
      bounds.ll.x = std::min(bounds.ll.x, itemBox.ll.x);
      bounds.ll.y = std::min(bounds.ll.y, itemBox.ll.y);
      bounds.ur.x = std::max(bounds.ur.x, itemBox.ur.x);
      bounds.ur.y = std::max(bounds.ur.y, itemBox.ur.y);
    }

    return bounds;
  }
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_LAYERCOSTANALYZER_H
