#include "visualization/CongestionHeatmap.h"
#include "board/Trace.h"
#include "board/Via.h"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace freerouting {

void CongestionHeatmap::analyze() {
  if (!board_) return;

  // Get board bounds
  boardBounds_ = IntBox(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);

  for (const auto& item : board_->getItems()) {
    IntBox itemBox = item->getBoundingBox();
    boardBounds_.ll.x = std::min(boardBounds_.ll.x, itemBox.ll.x);
    boardBounds_.ll.y = std::min(boardBounds_.ll.y, itemBox.ll.y);
    boardBounds_.ur.x = std::max(boardBounds_.ur.x, itemBox.ur.x);
    boardBounds_.ur.y = std::max(boardBounds_.ur.y, itemBox.ur.y);
  }

  // Convert grid size from mm to nanometers (1mm = 1,000,000 nm)
  int gridSizeNm = gridSizeMm_ * 1000000;

  // Calculate grid dimensions
  int boardWidth = boardBounds_.ur.x - boardBounds_.ll.x;
  int boardHeight = boardBounds_.ur.y - boardBounds_.ll.y;
  gridWidth_ = (boardWidth / gridSizeNm) + 1;
  gridHeight_ = (boardHeight / gridSizeNm) + 1;

  // Initialize grids for each layer
  int layerCount = board_->getLayers().count();
  for (int layer = 0; layer < layerCount; ++layer) {
    layerGrids_[layer] = std::vector<std::vector<GridCell>>(
      gridWidth_, std::vector<GridCell>(gridHeight_));
  }

  // Count traces and vias in each grid cell
  for (const auto& item : board_->getItems()) {
    const Trace* trace = dynamic_cast<const Trace*>(item.get());
    if (trace) {
      int layer = trace->getLayer();
      IntPoint start = trace->getStart();
      IntPoint end = trace->getEnd();

      // Rasterize trace into grid cells
      auto [x1, y1] = coordToGrid(start);
      auto [x2, y2] = coordToGrid(end);

      // Simple line rasterization (Bresenham)
      int dx = std::abs(x2 - x1);
      int dy = std::abs(y2 - y1);
      int sx = (x1 < x2) ? 1 : -1;
      int sy = (y1 < y2) ? 1 : -1;
      int err = dx - dy;

      int x = x1, y = y1;
      while (true) {
        if (x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_) {
          getCell(layer, x, y).traces++;
        }

        if (x == x2 && y == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
      }
    }

    const Via* via = dynamic_cast<const Via*>(item.get());
    if (via) {
      IntPoint center = via->getCenter();
      auto [x, y] = coordToGrid(center);

      // Vias affect all layers they span
      for (int layer = via->firstLayer(); layer <= via->lastLayer(); ++layer) {
        if (x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_) {
          getCell(layer, x, y).vias++;
        }
      }
    }
  }

  // Calculate congestion scores
  for (auto& [layer, grid] : layerGrids_) {
    for (auto& row : grid) {
      for (auto& cell : row) {
        cell.congestionScore = calculateCongestionScore(cell);
      }
    }
  }
}

void CongestionHeatmap::recordFailure(IntPoint location) {
  for (auto& [layer, grid] : layerGrids_) {
    auto [x, y] = coordToGrid(location);
    if (x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_) {
      getCell(layer, x, y).failures++;
      getCell(layer, x, y).congestionScore = calculateCongestionScore(getCell(layer, x, y));
    }
  }
}

void CongestionHeatmap::recordConflict(IntPoint location, int netNo) {
  (void)netNo;  // May use for per-net analysis later

  for (auto& [layer, grid] : layerGrids_) {
    auto [x, y] = coordToGrid(location);
    if (x >= 0 && x < gridWidth_ && y >= 0 && y < gridHeight_) {
      getCell(layer, x, y).conflicts++;
      getCell(layer, x, y).congestionScore = calculateCongestionScore(getCell(layer, x, y));
    }
  }
}

double CongestionHeatmap::getCongestionAt(IntPoint location, int layer) const {
  auto [x, y] = coordToGrid(location);
  if (x < 0 || x >= gridWidth_ || y < 0 || y >= gridHeight_) {
    return 0.0;
  }
  return getCell(layer, x, y).congestionScore;
}

std::vector<std::pair<IntPoint, double>> CongestionHeatmap::getMostCongestedAreas(int count) const {
  std::vector<std::pair<IntPoint, double>> hotspots;

  // Collect all cells with their locations and scores
  for (const auto& [layer, grid] : layerGrids_) {
    for (int x = 0; x < gridWidth_; ++x) {
      for (int y = 0; y < gridHeight_; ++y) {
        const auto& cell = grid[x][y];
        if (cell.congestionScore > 0.5) {  // Only include significantly congested cells
          // Convert grid coords back to board coords (cell center)
          int gridSizeNm = gridSizeMm_ * 1000000;
          IntPoint cellCenter(
            boardBounds_.ll.x + x * gridSizeNm + gridSizeNm / 2,
            boardBounds_.ll.y + y * gridSizeNm + gridSizeNm / 2
          );
          hotspots.emplace_back(cellCenter, cell.congestionScore);
        }
      }
    }
  }

  // Sort by congestion score (highest first)
  std::sort(hotspots.begin(), hotspots.end(),
    [](const auto& a, const auto& b) { return a.second > b.second; });

  // Return top N
  if (hotspots.size() > static_cast<size_t>(count)) {
    hotspots.resize(count);
  }

  return hotspots;
}

std::pair<int, int> CongestionHeatmap::coordToGrid(IntPoint point) const {
  int gridSizeNm = gridSizeMm_ * 1000000;
  int x = (point.x - boardBounds_.ll.x) / gridSizeNm;
  int y = (point.y - boardBounds_.ll.y) / gridSizeNm;
  return {x, y};
}

CongestionHeatmap::GridCell& CongestionHeatmap::getCell(int layer, int gridX, int gridY) {
  return layerGrids_[layer][gridX][gridY];
}

const CongestionHeatmap::GridCell& CongestionHeatmap::getCell(int layer, int gridX, int gridY) const {
  return layerGrids_.at(layer)[gridX][gridY];
}

double CongestionHeatmap::calculateCongestionScore(const GridCell& cell) const {
  // Congestion formula:
  // - Traces: 0.1 per trace (normal routing)
  // - Vias: 0.3 per via (vias are expensive)
  // - Failures: 1.0 per failure (critical indicator)
  // - Conflicts: 0.5 per conflict (moderate indicator)

  double score = cell.traces * 0.1 +
                 cell.vias * 0.3 +
                 cell.failures * 1.0 +
                 cell.conflicts * 0.5;

  return score;
}

std::string CongestionHeatmap::getHeatColor(double congestion) const {
  // Color scale: Blue (cold) -> Green -> Yellow -> Red (hot)
  if (congestion < 0.25) return "#0000FF";  // Blue
  if (congestion < 0.5) return "#00FF00";   // Green
  if (congestion < 1.0) return "#FFFF00";   // Yellow
  if (congestion < 2.0) return "#FF8000";   // Orange
  return "#FF0000";  // Red
}

void CongestionHeatmap::generateSVG(const std::string& filename, int layer) const {
  std::ofstream out(filename);
  if (!out) return;

  int gridSizeNm = gridSizeMm_ * 1000000;
  int svgWidth = gridWidth_ * 10;  // 10 pixels per grid cell
  int svgHeight = gridHeight_ * 10;

  out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  out << "<svg width=\"" << svgWidth << "\" height=\"" << svgHeight
      << "\" xmlns=\"http://www.w3.org/2000/svg\">\n";
  out << "  <title>Routing Congestion Heatmap - Layer " << layer << "</title>\n";

  // Draw grid cells
  for (int x = 0; x < gridWidth_; ++x) {
    for (int y = 0; y < gridHeight_; ++y) {
      double congestion = 0.0;

      if (layer >= 0 && layerGrids_.count(layer)) {
        congestion = layerGrids_.at(layer)[x][y].congestionScore;
      } else {
        // Average across all layers
        for (const auto& [l, grid] : layerGrids_) {
          congestion += grid[x][y].congestionScore;
        }
        if (!layerGrids_.empty()) {
          congestion /= layerGrids_.size();
        }
      }

      if (congestion > 0.1) {  // Only draw non-empty cells
        std::string color = getHeatColor(congestion);
        double opacity = std::min(1.0, congestion / 2.0);

        out << "  <rect x=\"" << x * 10 << "\" y=\"" << y * 10
            << "\" width=\"10\" height=\"10\""
            << " fill=\"" << color << "\""
            << " opacity=\"" << opacity << "\"/>\n";
      }
    }
  }

  out << "</svg>\n";
}

void CongestionHeatmap::generateReport(const std::string& filename) const {
  std::ofstream out(filename);
  if (!out) return;

  out << "Routing Congestion Report\n";
  out << "=========================\n\n";

  auto hotspots = getMostCongestedAreas(20);
  out << "Top 20 Most Congested Areas:\n\n";

  for (size_t i = 0; i < hotspots.size(); ++i) {
    const auto& [location, score] = hotspots[i];
    out << (i + 1) << ". Location ("
        << (location.x / 1000000.0) << "mm, "
        << (location.y / 1000000.0) << "mm) - Score: "
        << score << "\n";
  }

  out << "\nRecommendation: Consider moving components near these hotspots\n";
  out << "to reduce congestion and improve routing success.\n";
}

} // namespace freerouting
