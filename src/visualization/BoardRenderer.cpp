#include "visualization/BoardRenderer.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "board/Pin.h"
#include "board/Component.h"
#include "board/BoardOutline.h"
#include "io/KiCadPcb.h"
#include <algorithm>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace freerouting {

// KiCad 9 dark theme color palette
namespace KiCadColors {
  constexpr SDL_Color Background{26, 26, 26, 255};        // Dark gray background
  constexpr SDL_Color Grid{50, 50, 50, 255};             // Subtle grid
  constexpr SDL_Color BoardEdge{180, 180, 50, 255};      // Muted yellow-green board outline
  constexpr SDL_Color CopperFront{200, 52, 52, 255};     // Red for F.Cu
  constexpr SDL_Color CopperBack{0, 132, 0, 255};        // Green for B.Cu
  constexpr SDL_Color CopperInner{200, 200, 0, 255};     // Yellow for inner layers
  constexpr SDL_Color PadCopper{220, 200, 80, 255};      // Brighter gold for pads
  constexpr SDL_Color PadHole{50, 50, 50, 255};          // Dark for holes
  constexpr SDL_Color Via{236, 236, 236, 255};           // Light gray for vias
  constexpr SDL_Color SilkscreenFront{230, 230, 230, 255};  // White silkscreen
  constexpr SDL_Color ComponentOutline{132, 0, 132, 255};   // Purple for components
  constexpr SDL_Color RatsnestLine{255, 255, 0, 128};    // Semi-transparent yellow
}


BoardRenderer::BoardRenderer(RoutingBoard* board, const std::string& title)
  : board_(board), title_(title), heatmap_(board) {
}

BoardRenderer::~BoardRenderer() {
  shutdown();
}

bool BoardRenderer::initialize() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    return false;
  }

  window_ = SDL_CreateWindow(
    title_.c_str(),
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    config_.windowWidth,
    config_.windowHeight,
    SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
  );

  if (!window_) {
    SDL_Quit();
    return false;
  }

  renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (!renderer_) {
    SDL_DestroyWindow(window_);
    SDL_Quit();
    return false;
  }

  // Calculate board bounds and scale
  boardBounds_ = IntBox(INT32_MAX, INT32_MAX, INT32_MIN, INT32_MIN);
  for (const auto& item : board_->getItems()) {
    IntBox itemBox = item->getBoundingBox();
    boardBounds_.ll.x = std::min(boardBounds_.ll.x, itemBox.ll.x);
    boardBounds_.ll.y = std::min(boardBounds_.ll.y, itemBox.ll.y);
    boardBounds_.ur.x = std::max(boardBounds_.ur.x, itemBox.ur.x);
    boardBounds_.ur.y = std::max(boardBounds_.ur.y, itemBox.ur.y);
  }

  calculateScale();

  // Initialize heatmap for congestion overlay
  heatmap_.analyze();

  return true;
}

void BoardRenderer::shutdown() {
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = nullptr;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = nullptr;
  }
  SDL_Quit();
}

void BoardRenderer::render() {
  if (!renderer_) return;

  std::lock_guard<std::mutex> lock(renderMutex_);

  renderBackground();

  if (config_.showGrid) {
    renderGrid();
  }

  // Render in KiCad-like order: board outline, components, pads, traces, vias, overlays
  renderBoardOutline();

  renderComponents();

  if (config_.showPads) {
    renderPads();
  }

  if (config_.showTraces) {
    renderTraces();
  }

  if (config_.showVias) {
    renderVias();
  }

  // Render overlay based on mode
  renderOverlay();

  // Render UI info
  renderUI();

  SDL_RenderPresent(renderer_);
  needsRedraw_ = false;
}

bool BoardRenderer::processEvents() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        return false;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
          case SDLK_q:
            return false;

          case SDLK_EQUALS:
          case SDLK_PLUS: {
            // Zoom centered on window center for keyboard zoom
            SDL_Point center = {config_.windowWidth / 2, config_.windowHeight / 2};
            zoomIn(center);
            needsRedraw_ = true;
            break;
          }

          case SDLK_MINUS: {
            // Zoom centered on window center for keyboard zoom
            SDL_Point center = {config_.windowWidth / 2, config_.windowHeight / 2};
            zoomOut(center);
            needsRedraw_ = true;
            break;
          }

          case SDLK_c:
            setOverlayMode(OverlayMode::Congestion);
            needsRedraw_ = true;
            break;

          case SDLK_f:
            setOverlayMode(OverlayMode::Failures);
            needsRedraw_ = true;
            break;

          case SDLK_r:
            setOverlayMode(OverlayMode::Ripups);
            needsRedraw_ = true;
            break;

          case SDLK_n:
            setOverlayMode(OverlayMode::None);
            needsRedraw_ = true;
            break;

          case SDLK_g:
            config_.showGrid = !config_.showGrid;
            needsRedraw_ = true;
            break;

          case SDLK_UP:
            pan(0, 50);
            needsRedraw_ = true;
            break;

          case SDLK_DOWN:
            pan(0, -50);
            needsRedraw_ = true;
            break;

          case SDLK_LEFT:
            pan(50, 0);
            needsRedraw_ = true;
            break;

          case SDLK_RIGHT:
            pan(-50, 0);
            needsRedraw_ = true;
            break;

          case SDLK_HOME:
            // Reset to fit view
            config_.zoomLevel = 1.0;
            config_.panOffset = IntPoint(0, 0);
            calculateScale();
            needsRedraw_ = true;
            break;
        }
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          config_.windowWidth = event.window.data1;
          config_.windowHeight = event.window.data2;
          calculateScale();
          needsRedraw_ = true;
        }
        break;

      case SDL_MOUSEWHEEL: {
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);
        SDL_Point mousePos = {mouseX, mouseY};
        if (event.wheel.y > 0) {
          zoomIn(mousePos);
        } else if (event.wheel.y < 0) {
          zoomOut(mousePos);
        }
        needsRedraw_ = true;
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_RIGHT) {
          // Start right-drag for panning
          SDL_CaptureMouse(SDL_TRUE);
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_RIGHT) {
          SDL_CaptureMouse(SDL_FALSE);
        }
        break;

      case SDL_MOUSEMOTION:
        if (event.motion.state & SDL_BUTTON_RMASK) {
          // Right mouse button drag - pan the view
          pan(event.motion.xrel, event.motion.yrel);
          needsRedraw_ = true;
        }
        break;
    }
  }

  return true;
}

void BoardRenderer::notifyTraceAdded(const Trace* trace) {
  (void)trace;
  std::lock_guard<std::mutex> lock(renderMutex_);
  needsRedraw_ = true;
}

void BoardRenderer::notifyViaAdded(const Via* via) {
  (void)via;
  std::lock_guard<std::mutex> lock(renderMutex_);
  needsRedraw_ = true;
}

void BoardRenderer::notifyRoutingFailed(IntPoint location, int netNo) {
  std::lock_guard<std::mutex> lock(renderMutex_);
  heatmap_.recordFailure(location);
  (void)netNo;
  needsRedraw_ = true;
}

void BoardRenderer::notifyRipup(IntPoint location, int netNo) {
  std::lock_guard<std::mutex> lock(renderMutex_);
  heatmap_.recordConflict(location, netNo);
  needsRedraw_ = true;
}

SDL_Point BoardRenderer::boardToScreen(IntPoint point) const {
  // Convert nanometers to screen pixels with zoom and pan
  double x = (point.x - boardBounds_.ll.x) * scale_ * config_.zoomLevel + config_.panOffset.x;
  double y = (point.y - boardBounds_.ll.y) * scale_ * config_.zoomLevel + config_.panOffset.y;
  return {static_cast<int>(x), static_cast<int>(y)};
}

IntPoint BoardRenderer::screenToBoard(SDL_Point point) const {
  // Convert screen pixels back to nanometers
  double x = (point.x - config_.panOffset.x) / (scale_ * config_.zoomLevel) + boardBounds_.ll.x;
  double y = (point.y - config_.panOffset.y) / (scale_ * config_.zoomLevel) + boardBounds_.ll.y;
  return IntPoint(static_cast<int>(x), static_cast<int>(y));
}

void BoardRenderer::calculateScale() {
  int boardWidth = boardBounds_.ur.x - boardBounds_.ll.x;
  int boardHeight = boardBounds_.ur.y - boardBounds_.ll.y;

  if (boardWidth == 0 || boardHeight == 0) {
    scale_ = 1.0;
    return;
  }

  // Add 10% padding
  double scaleX = (config_.windowWidth * 0.9) / boardWidth;
  double scaleY = (config_.windowHeight * 0.9) / boardHeight;

  scale_ = std::min(scaleX, scaleY);

  // Center the board
  config_.panOffset.x = (config_.windowWidth - boardWidth * scale_) / 2;
  config_.panOffset.y = (config_.windowHeight - boardHeight * scale_) / 2;
}

void BoardRenderer::renderBackground() {
  auto& bg = KiCadColors::Background;
  SDL_SetRenderDrawColor(renderer_, bg.r, bg.g, bg.b, bg.a);
  SDL_RenderClear(renderer_);
}

void BoardRenderer::renderGrid() {
  auto& grid = KiCadColors::Grid;
  SDL_SetRenderDrawColor(renderer_, grid.r, grid.g, grid.b, grid.a);

  // Grid spacing in nanometers (5mm)
  int gridSpacing = 5000000;

  int startX = (boardBounds_.ll.x / gridSpacing) * gridSpacing;
  int startY = (boardBounds_.ll.y / gridSpacing) * gridSpacing;

  // Vertical lines
  for (int x = startX; x <= boardBounds_.ur.x; x += gridSpacing) {
    SDL_Point p1 = boardToScreen(IntPoint(x, boardBounds_.ll.y));
    SDL_Point p2 = boardToScreen(IntPoint(x, boardBounds_.ur.y));
    SDL_RenderDrawLine(renderer_, p1.x, p1.y, p2.x, p2.y);
  }

  // Horizontal lines
  for (int y = startY; y <= boardBounds_.ur.y; y += gridSpacing) {
    SDL_Point p1 = boardToScreen(IntPoint(boardBounds_.ll.x, y));
    SDL_Point p2 = boardToScreen(IntPoint(boardBounds_.ur.x, y));
    SDL_RenderDrawLine(renderer_, p1.x, p1.y, p2.x, p2.y);
  }
}

void BoardRenderer::renderBoardOutline() {
  auto& edge = KiCadColors::BoardEdge;
  SDL_SetRenderDrawColor(renderer_, edge.r, edge.g, edge.b, edge.a);

  // Draw board bounds as yellow rectangle
  SDL_Point ll = boardToScreen(boardBounds_.ll);
  SDL_Point ur = boardToScreen(boardBounds_.ur);

  // Draw thick outline (3 pixels)
  for (int offset = -1; offset <= 1; ++offset) {
    SDL_Rect rect = {ll.x + offset, ll.y + offset,
                     ur.x - ll.x - 2*offset, ur.y - ll.y - 2*offset};
    SDL_RenderDrawRect(renderer_, &rect);
  }
}

void BoardRenderer::renderComponents() {
  try {
    auto footprintsPtr = board_->getFootprints();
    if (!footprintsPtr) {
      static bool logged = false;
      if (!logged) {
        std::cerr << "DEBUG: No footprints pointer" << std::endl;
        logged = true;
      }
      return;  // No footprints loaded
    }

    static bool logged = false;
    if (!logged) {
      std::cerr << "DEBUG: Found " << footprintsPtr->size() << " footprints" << std::endl;
      int totalLines = 0;
      for (const auto& fp : *footprintsPtr) {
        totalLines += fp.fpLines.size();
      }
      std::cerr << "DEBUG: Total fp_lines across all footprints: " << totalLines << std::endl;
      logged = true;
    }

    for (const auto& footprint : *footprintsPtr) {
      // Transform footprint graphics to board coordinates
      double cosR = std::cos(footprint.rotation * M_PI / 180.0);
      double sinR = std::sin(footprint.rotation * M_PI / 180.0);

      // Render fp_line elements (courtyards and silkscreen)
      static int debugCount = 0;
      for (const auto& line : footprint.fpLines) {
        // Skip non-courtyard/silkscreen layers for now
        if (line.layer != "F.CrtYd" && line.layer != "B.CrtYd" &&
            line.layer != "F.SilkS" && line.layer != "B.SilkS") {
          continue;
        }

        // Transform line coordinates from footprint space to board space
        // Apply rotation and translation
        double x1 = line.startX * cosR - line.startY * sinR + footprint.x;
        double y1 = line.startX * sinR + line.startY * cosR + footprint.y;
        double x2 = line.endX * cosR - line.endY * sinR + footprint.x;
        double y2 = line.endX * sinR + line.endY * cosR + footprint.y;

        // Convert millimeters to internal units (10000 units per mm)
        IntPoint start(static_cast<int>(x1 * 10000), static_cast<int>(y1 * 10000));
        IntPoint end(static_cast<int>(x2 * 10000), static_cast<int>(y2 * 10000));

        // Convert to screen coordinates
        SDL_Point screenStart = boardToScreen(start);
        SDL_Point screenEnd = boardToScreen(end);

        if (debugCount < 5) {
          std::cerr << "DEBUG: Line " << debugCount << " layer=" << line.layer
                    << " mm:(" << x1 << "," << y1 << ")->(" << x2 << "," << y2 << ")"
                    << " screen:(" << screenStart.x << "," << screenStart.y << ")->("
                    << screenEnd.x << "," << screenEnd.y << ")" << std::endl;
          debugCount++;
        }

        // Choose color based on layer
        SDL_Color color;
        if (line.layer == "F.CrtYd" || line.layer == "B.CrtYd") {
          color = KiCadColors::ComponentOutline;  // Purple for courtyards
        } else {
          color = KiCadColors::SilkscreenFront;   // White for silkscreen
        }

        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
        SDL_RenderDrawLine(renderer_, screenStart.x, screenStart.y, screenEnd.x, screenEnd.y);
      }

      // TODO: Render fp_text for reference designators and values
    }
  } catch (...) {
    // Silently handle exceptions from concurrent access
  }
}

void BoardRenderer::renderPads() {
  try {
    auto& holeColor = KiCadColors::PadHole;
    SDL_Color outlineColor = {180, 180, 180, 255};  // Light gray outline

    const auto& items = board_->getItems();
    const auto& layers = board_->getLayers();
    int lastLayer = layers.count() - 1;

    for (size_t i = 0; i < items.size(); ++i) {
      if (!items[i]) continue;  // Safety check
      const Pin* pin = dynamic_cast<const Pin*>(items[i].get());
      if (!pin) continue;

      // Show pin based on active layer filter
      if (activeLayer_ >= 0 && (pin->firstLayer() > activeLayer_ || pin->lastLayer() < activeLayer_)) {
        continue;
      }

      IntPoint center = pin->getCenter();
      SDL_Point screenCenter = boardToScreen(center);

      // Get the bounding box to determine actual pad size
      IntBox pinBox = pin->getBoundingBox();
      int pinWidth = pinBox.ur.x - pinBox.ll.x;
      int pinHeight = pinBox.ur.y - pinBox.ll.y;

      // Use the larger dimension for circular approximation
      int pinDiameter = std::max(pinWidth, pinHeight);
      int padRadiusPixels = static_cast<int>((pinDiameter / 2.0) * scale_ * config_.zoomLevel);

      // Don't clamp to max size - let pads show their actual size
      if (padRadiusPixels < 2) padRadiusPixels = 2;

      // Determine pad color based on layer (same as traces)
      int layer = pin->firstLayer();
      SDL_Color padColor;
      if (layer == 0) {
        padColor = KiCadColors::CopperFront;  // F.Cu (red)
      } else if (layer == lastLayer) {
        padColor = KiCadColors::CopperBack;   // B.Cu (green)
      } else {
        padColor = KiCadColors::CopperInner;  // Inner layers (yellow)
      }

      // Draw copper pad as filled rectangle (approximation of circle)
      SDL_SetRenderDrawColor(renderer_, padColor.r, padColor.g, padColor.b, padColor.a);
      SDL_Rect padRect = {screenCenter.x - padRadiusPixels, screenCenter.y - padRadiusPixels,
                          padRadiusPixels * 2, padRadiusPixels * 2};
      SDL_RenderFillRect(renderer_, &padRect);

      // Draw light gray outline
      SDL_SetRenderDrawColor(renderer_, outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
      SDL_RenderDrawRect(renderer_, &padRect);

      // Draw drill hole (darker, smaller) - about 50% of pad size
      int holeRadiusPixels = padRadiusPixels / 2;
      if (holeRadiusPixels < 1) holeRadiusPixels = 1;

      SDL_SetRenderDrawColor(renderer_, holeColor.r, holeColor.g, holeColor.b, holeColor.a);
      SDL_Rect holeRect = {screenCenter.x - holeRadiusPixels, screenCenter.y - holeRadiusPixels,
                           holeRadiusPixels * 2, holeRadiusPixels * 2};
      SDL_RenderFillRect(renderer_, &holeRect);
    }
  } catch (...) {
    // Silently handle exceptions from concurrent access
  }
}

void BoardRenderer::renderTraces() {
  try {
    const auto& layers = board_->getLayers();
    int lastLayer = layers.count() - 1;

    const auto& items = board_->getItems();
    for (size_t i = 0; i < items.size(); ++i) {
      if (!items[i]) continue;  // Safety check
      const Trace* trace = dynamic_cast<const Trace*>(items[i].get());
      if (trace) {
        int layer = trace->getLayer();

        // For now, only render outer signal layers (F.Cu and B.Cu)
        // Skip inner layers which may be power/ground planes or additional signals
        // This prevents the orange background from filled copper pours
        if (layer == 0 || layer == lastLayer) {
          drawTrace(trace);
        }
        // TODO: Add layer filtering UI to selectively show inner layers
      }
    }
  } catch (...) {
    // Silently handle exceptions from concurrent access
  }
}

void BoardRenderer::renderVias() {
  try {
    const auto& items = board_->getItems();
    for (size_t i = 0; i < items.size(); ++i) {
      if (!items[i]) continue;  // Safety check
      const Via* via = dynamic_cast<const Via*>(items[i].get());
      if (via) {
        drawVia(via);
      }
    }
  } catch (...) {
    // Silently handle exceptions from concurrent access
  }
}

void BoardRenderer::renderOverlay() {
  switch (overlayMode_) {
    case OverlayMode::Congestion:
      renderCongestionHeatmap();
      break;
    case OverlayMode::Failures:
      renderFailurePoints();
      break;
    case OverlayMode::Ripups:
      renderRipupPoints();
      break;
    default:
      break;
  }
}

void BoardRenderer::renderCongestionHeatmap() {
  // Re-analyze heatmap with current board state
  heatmap_.analyze();

  // Draw congestion as colored rectangles
  int gridSizeMm = 5;
  int gridSizeNm = gridSizeMm * 1000000;

  int boardWidth = boardBounds_.ur.x - boardBounds_.ll.x;
  int boardHeight = boardBounds_.ur.y - boardBounds_.ll.y;
  int gridWidth = (boardWidth / gridSizeNm) + 1;
  int gridHeight = (boardHeight / gridSizeNm) + 1;

  for (int x = 0; x < gridWidth; ++x) {
    for (int y = 0; y < gridHeight; ++y) {
      IntPoint cellLL(boardBounds_.ll.x + x * gridSizeNm, boardBounds_.ll.y + y * gridSizeNm);
      IntPoint cellUR(cellLL.x + gridSizeNm, cellLL.y + gridSizeNm);

      double congestion = heatmap_.getCongestionAt(
        IntPoint((cellLL.x + cellUR.x) / 2, (cellLL.y + cellUR.y) / 2),
        activeLayer_ >= 0 ? activeLayer_ : 0
      );

      if (congestion > 0.1) {
        SDL_Point p1 = boardToScreen(cellLL);
        SDL_Point p2 = boardToScreen(cellUR);

        SDL_Color color = getHeatColor(congestion);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, 128);  // Semi-transparent

        SDL_Rect rect = {p1.x, p1.y, p2.x - p1.x, p2.y - p1.y};
        SDL_RenderFillRect(renderer_, &rect);
      }
    }
  }
}

void BoardRenderer::renderFailurePoints() {
  // Show routing failure locations as red X marks
  auto hotspots = heatmap_.getMostCongestedAreas(50);

  SDL_SetRenderDrawColor(renderer_, 255, 0, 0, 255);  // Red
  for (const auto& [location, score] : hotspots) {
    if (score < 1.0) continue;  // Only show actual failures

    SDL_Point p = boardToScreen(location);
    int size = 5;

    // Draw X
    SDL_RenderDrawLine(renderer_, p.x - size, p.y - size, p.x + size, p.y + size);
    SDL_RenderDrawLine(renderer_, p.x - size, p.y + size, p.x + size, p.y - size);
  }
}

void BoardRenderer::renderRipupPoints() {
  // Similar to failure points but show conflicts (yellow)
  auto hotspots = heatmap_.getMostCongestedAreas(50);

  SDL_SetRenderDrawColor(renderer_, 255, 255, 0, 255);  // Yellow
  for (const auto& [location, score] : hotspots) {
    SDL_Point p = boardToScreen(location);
    int size = 3;

    // Draw small circle
    SDL_Rect rect = {p.x - size, p.y - size, size * 2, size * 2};
    SDL_RenderFillRect(renderer_, &rect);
  }
}

void BoardRenderer::renderUI() {
  // Draw UI overlay with keyboard shortcuts and info
  // For now, just draw a simple status bar
  SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
  SDL_Rect statusBar = {0, config_.windowHeight - 30, config_.windowWidth, 30};
  SDL_RenderFillRect(renderer_, &statusBar);

  // Text rendering would require SDL_ttf, so we'll skip for now
  // In a full implementation, show: zoom level, overlay mode, layer, controls
}

void BoardRenderer::drawTrace(const Trace* trace) {
  if (!trace) return;

  // Show trace based on active layer filter
  if (activeLayer_ >= 0 && trace->getLayer() != activeLayer_) {
    return;
  }

  IntPoint start = trace->getStart();
  IntPoint end = trace->getEnd();

  SDL_Point p1 = boardToScreen(start);
  SDL_Point p2 = boardToScreen(end);

  // Get layer-appropriate color (KiCad style)
  int layer = trace->getLayer();
  SDL_Color color;
  if (layer == 0) {
    color = KiCadColors::CopperFront;  // F.Cu (red)
  } else if (layer == board_->getLayers().count() - 1) {
    color = KiCadColors::CopperBack;   // B.Cu (green)
  } else {
    color = KiCadColors::CopperInner;  // Inner layers (yellow)
  }

  SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);

  // Calculate trace width in screen space (use actual physical width)
  int halfWidth = static_cast<int>(trace->getHalfWidth() * scale_ * config_.zoomLevel);
  if (halfWidth < 1) halfWidth = 1;

  // Draw trace as thick line using SDL line thickness
  // For very thin traces at low zoom, just draw a single line
  if (halfWidth <= 1) {
    SDL_RenderDrawLine(renderer_, p1.x, p1.y, p2.x, p2.y);
    return;
  }

  // For thicker traces, draw as filled rectangle with perpendicular edges
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double len = std::sqrt(dx * dx + dy * dy);

  if (len < 0.001) {
    // Degenerate trace (start == end), draw as a small filled square
    SDL_Rect rect = {p1.x - halfWidth, p1.y - halfWidth, halfWidth * 2, halfWidth * 2};
    SDL_RenderFillRect(renderer_, &rect);
    return;
  }

  // Calculate perpendicular offset vector (normalized and scaled by halfWidth)
  double perpX = -dy / len * halfWidth;
  double perpY = dx / len * halfWidth;

  // Four corners of the rectangle/parallelogram
  SDL_Point corners[4];
  corners[0] = {p1.x + static_cast<int>(perpX), p1.y + static_cast<int>(perpY)};
  corners[1] = {p2.x + static_cast<int>(perpX), p2.y + static_cast<int>(perpY)};
  corners[2] = {p2.x - static_cast<int>(perpX), p2.y - static_cast<int>(perpY)};
  corners[3] = {p1.x - static_cast<int>(perpX), p1.y - static_cast<int>(perpY)};

  // Draw filled polygon using scanline approach
  int minY = corners[0].y;
  int maxY = corners[0].y;
  for (int i = 1; i < 4; ++i) {
    minY = std::min(minY, corners[i].y);
    maxY = std::max(maxY, corners[i].y);
  }

  for (int y = minY; y <= maxY; ++y) {
    int intersections[4];
    int count = 0;

    // Find intersections with all 4 edges
    for (int i = 0; i < 4; ++i) {
      const SDL_Point& a = corners[i];
      const SDL_Point& b = corners[(i + 1) % 4];

      if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
        if (a.y != b.y) {
          int x = a.x + (b.x - a.x) * (y - a.y) / (b.y - a.y);
          intersections[count++] = x;
        }
      }
    }

    // Sort intersections and draw line between pairs
    if (count >= 2) {
      for (int i = 0; i < count - 1; ++i) {
        for (int j = i + 1; j < count; ++j) {
          if (intersections[i] > intersections[j]) {
            int tmp = intersections[i];
            intersections[i] = intersections[j];
            intersections[j] = tmp;
          }
        }
      }
      SDL_RenderDrawLine(renderer_, intersections[0], y, intersections[count-1], y);
    }
  }
}

void BoardRenderer::drawVia(const Via* via) {
  if (!via) return;

  IntPoint center = via->getCenter();
  SDL_Point screenCenter = boardToScreen(center);

  // Use realistic via size (0.8mm radius = 800000 nm)
  int radius = static_cast<int>(800000 * scale_ * config_.zoomLevel);
  if (radius < 2) radius = 2;

  // Draw via annular ring (light gray)
  auto& viaColor = KiCadColors::Via;
  SDL_SetRenderDrawColor(renderer_, viaColor.r, viaColor.g, viaColor.b, viaColor.a);
  SDL_Rect rect = {screenCenter.x - radius, screenCenter.y - radius, radius * 2, radius * 2};
  SDL_RenderFillRect(renderer_, &rect);

  // Draw drill hole (dark)
  int drillRadius = radius / 2;
  auto& holeColor = KiCadColors::PadHole;
  SDL_SetRenderDrawColor(renderer_, holeColor.r, holeColor.g, holeColor.b, holeColor.a);
  SDL_Rect drillRect = {screenCenter.x - drillRadius, screenCenter.y - drillRadius,
                        drillRadius * 2, drillRadius * 2};
  SDL_RenderFillRect(renderer_, &drillRect);
}

SDL_Color BoardRenderer::getNetColor(int netNo) const {
  // Hash net number to get consistent color
  unsigned int hash = static_cast<unsigned int>(netNo);
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;

  uint8_t r = 80 + (hash & 0xFF) % 176;
  uint8_t g = 80 + ((hash >> 8) & 0xFF) % 176;
  uint8_t b = 80 + ((hash >> 16) & 0xFF) % 176;

  return {r, g, b, 255};
}

SDL_Color BoardRenderer::getHeatColor(double congestion) const {
  // Color scale: Blue (cold) -> Green -> Yellow -> Red (hot)
  if (congestion < 0.25) return {0, 0, 255, 255};      // Blue
  if (congestion < 0.5) return {0, 255, 0, 255};       // Green
  if (congestion < 1.0) return {255, 255, 0, 255};     // Yellow
  if (congestion < 2.0) return {255, 128, 0, 255};     // Orange
  return {255, 0, 0, 255};                              // Red
}

void BoardRenderer::zoomIn(SDL_Point mousePos) {
  // Convert mouse position to board coordinates before zoom
  IntPoint boardPos = screenToBoard(mousePos);

  // Apply zoom
  config_.zoomLevel *= 1.2;

  // Convert same board position to new screen coordinates after zoom
  SDL_Point newScreenPos = boardToScreen(boardPos);

  // Adjust pan offset so board position stays under mouse
  config_.panOffset.x += mousePos.x - newScreenPos.x;
  config_.panOffset.y += mousePos.y - newScreenPos.y;
}

void BoardRenderer::zoomOut(SDL_Point mousePos) {
  // Convert mouse position to board coordinates before zoom
  IntPoint boardPos = screenToBoard(mousePos);

  // Apply zoom
  config_.zoomLevel /= 1.2;

  // Convert same board position to new screen coordinates after zoom
  SDL_Point newScreenPos = boardToScreen(boardPos);

  // Adjust pan offset so board position stays under mouse
  config_.panOffset.x += mousePos.x - newScreenPos.x;
  config_.panOffset.y += mousePos.y - newScreenPos.y;
}

} // namespace freerouting
