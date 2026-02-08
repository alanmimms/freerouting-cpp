#ifndef FREEROUTING_VISUALIZATION_CONGESTIONHEATMAP_H
#define FREEROUTING_VISUALIZATION_CONGESTIONHEATMAP_H

#include "board/RoutingBoard.h"
#include "geometry/IntBox.h"
#include <string>
#include <vector>
#include <map>

namespace freerouting {

// Generates congestion heatmap to visualize routing bottlenecks
// Helps identify areas where component placement should be adjusted
class CongestionHeatmap {
public:
  // Grid cell for congestion tracking
  struct GridCell {
    int traces = 0;           // Number of trace segments
    int vias = 0;             // Number of vias
    int failures = 0;         // Number of failed routing attempts
    int conflicts = 0;        // Number of DRC conflicts
    double congestionScore = 0.0;  // Combined congestion metric
  };

  CongestionHeatmap(RoutingBoard* board, int gridSizeMm = 5)
    : board_(board), gridSizeMm_(gridSizeMm) {}

  // Analyze board and build congestion data
  void analyze();

  // Track a failed routing attempt at a location
  void recordFailure(IntPoint location);

  // Track a conflict/ripup at a location
  void recordConflict(IntPoint location, int netNo);

  // Generate SVG heatmap file
  void generateSVG(const std::string& filename, int layer = -1) const;

  // Generate text report of most congested areas
  void generateReport(const std::string& filename) const;

  // Get congestion score for a specific location (0.0 = empty, 1.0+ = congested)
  double getCongestionAt(IntPoint location, int layer) const;

  // Get list of most congested cells (for manual review)
  std::vector<std::pair<IntPoint, double>> getMostCongestedAreas(int count = 10) const;

private:
  RoutingBoard* board_;
  int gridSizeMm_;  // Grid cell size in millimeters

  // Per-layer congestion grids
  std::map<int, std::vector<std::vector<GridCell>>> layerGrids_;

  // Board bounds
  IntBox boardBounds_;
  int gridWidth_ = 0;
  int gridHeight_ = 0;

  // Convert board coordinates to grid coordinates
  std::pair<int, int> coordToGrid(IntPoint point) const;

  // Get grid cell (creates if doesn't exist)
  GridCell& getCell(int layer, int gridX, int gridY);
  const GridCell& getCell(int layer, int gridX, int gridY) const;

  // Calculate congestion score for a cell
  double calculateCongestionScore(const GridCell& cell) const;

  // Get color for SVG based on congestion level
  std::string getHeatColor(double congestion) const;
};

} // namespace freerouting

#endif // FREEROUTING_VISUALIZATION_CONGESTIONHEATMAP_H
