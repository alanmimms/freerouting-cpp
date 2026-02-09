#ifndef FREEROUTING_VISUALIZATION_BOARDRENDERER_H
#define FREEROUTING_VISUALIZATION_BOARDRENDERER_H

#include "board/RoutingBoard.h"
#include "geometry/IntBox.h"
#include "visualization/CongestionHeatmap.h"
#include <SDL2/SDL.h>
#include <string>
#include <memory>
#include <atomic>
#include <mutex>

namespace freerouting {

// Real-time board visualization with multiple overlay modes
// Shows routing progress, congestion, and allows interactive inspection
class BoardRenderer {
public:
  // Overlay visualization modes
  enum class OverlayMode {
    None,              // Just show the board
    Congestion,        // Show congestion heatmap
    Failures,          // Show failed routing attempts
    Ripups,            // Show ripup locations
    NetHighlight,      // Highlight specific net
    LayerView          // Show specific layer only
  };

  // Rendering configuration
  struct Config {
    int windowWidth = 1920;
    int windowHeight = 1080;
    bool showGrid = true;
    bool showPads = true;
    bool showTraces = true;
    bool showVias = true;
    bool showLabels = false;
    int targetFPS = 30;
    double zoomLevel = 1.0;
    IntPoint panOffset{0, 0};
  };

  BoardRenderer(RoutingBoard* board, const std::string& title = "FreeRouting Visualization");
  ~BoardRenderer();

  // Initialize SDL and create window
  bool initialize();

  // Shutdown and cleanup
  void shutdown();

  // Update and render one frame
  void render();

  // Process SDL events (returns false if window closed)
  bool processEvents();

  // Update routing progress (called by autorouter)
  void notifyTraceAdded(const class Trace* trace);
  void notifyViaAdded(const class Via* via);
  void notifyRoutingFailed(IntPoint location, int netNo);
  void notifyRipup(IntPoint location, int netNo);

  // Set overlay mode
  void setOverlayMode(OverlayMode mode) { overlayMode_ = mode; }

  // Set specific layer to view (-1 for all layers)
  void setActiveLayer(int layer) { activeLayer_ = layer; }

  // Zoom and pan controls
  void zoomIn() { config_.zoomLevel *= 1.2; }
  void zoomOut() { config_.zoomLevel /= 1.2; }
  void pan(int dx, int dy) {
    config_.panOffset.x += dx;
    config_.panOffset.y += dy;
  }

  // Get current configuration
  const Config& getConfig() const { return config_; }
  Config& getConfig() { return config_; }

  // Check if renderer is active
  bool isActive() const { return window_ != nullptr; }

  // Get congestion heatmap (for manual analysis)
  const CongestionHeatmap& getHeatmap() const { return heatmap_; }

private:
  RoutingBoard* board_;
  std::string title_;
  Config config_;
  OverlayMode overlayMode_ = OverlayMode::None;
  int activeLayer_ = -1;  // -1 means all layers

  // SDL resources
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;

  // Congestion tracking
  CongestionHeatmap heatmap_;

  // Thread safety for updates from autorouter thread
  std::mutex renderMutex_;
  std::atomic<bool> needsRedraw_{true};

  // Board coordinate system
  IntBox boardBounds_;
  double scale_ = 1.0;  // Nanometers to pixels

  // Convert board coordinates to screen coordinates
  SDL_Point boardToScreen(IntPoint point) const;
  IntPoint screenToBoard(SDL_Point point) const;

  // Calculate appropriate scale factor
  void calculateScale();

  // Rendering functions
  void renderBackground();
  void renderGrid();
  void renderBoardOutline();
  void renderComponents();
  void renderPads();
  void renderTraces();
  void renderVias();
  void renderOverlay();
  void renderCongestionHeatmap();
  void renderFailurePoints();
  void renderRipupPoints();
  void renderUI();

  // Helper: draw a trace segment
  void drawTrace(const class Trace* trace);

  // Helper: draw a via
  void drawVia(const class Via* via);

  // Helper: get color for a net
  SDL_Color getNetColor(int netNo) const;

  // Helper: get color for congestion level
  SDL_Color getHeatColor(double congestion) const;
};

} // namespace freerouting

#endif // FREEROUTING_VISUALIZATION_BOARDRENDERER_H
