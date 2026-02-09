#ifndef FREEROUTING_IO_KICADBOARDCONVERTER_H
#define FREEROUTING_IO_KICADBOARDCONVERTER_H

#include "io/KiCadPcb.h"
#include "board/RoutingBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "board/Component.h"
#include "board/Pin.h"
#include "core/Padstack.h"
#include "rules/ClearanceMatrix.h"
#include "rules/Nets.h"
#include "geometry/Vector2.h"
#include <memory>
#include <cmath>

namespace freerouting {

// Converter between KiCad PCB format and RoutingBoard
// Handles unit conversion and object creation
class KiCadBoardConverter {
public:
  // Unit conversion: 1 mm = 10,000 internal units (0.1 micrometer resolution)
  // This gives us:
  // - Max board size: ~3.3 meters (with kCritInt = 2^25)
  // - Resolution: 0.1 micrometers = 100 nanometers
  static constexpr int kUnitsPerMm = 10000;

  // Convert millimeters to internal units
  static int mmToUnits(double mm) {
    return static_cast<int>(std::round(mm * kUnitsPerMm));
  }

  // Convert internal units to millimeters
  static double unitsToMm(int units) {
    return static_cast<double>(units) / kUnitsPerMm;
  }

  // Convert KiCad point (mm) to IntPoint (internal units)
  static IntPoint convertPoint(double x, double y) {
    return IntPoint(mmToUnits(x), mmToUnits(y));
  }

  // Convert IntPoint (internal units) to KiCad point (mm)
  static void convertPointBack(IntPoint point, double& x, double& y) {
    x = unitsToMm(point.x);
    y = unitsToMm(point.y);
  }

  // Create RoutingBoard from KiCadPcb
  // Returns a pair of (board, clearanceMatrix) because board stores a pointer to the matrix
  static std::pair<std::unique_ptr<RoutingBoard>, std::unique_ptr<ClearanceMatrix>>
  createRoutingBoard(const KiCadPcb& kicadPcb) {
    // Create clearance matrix (must outlive board since board stores a pointer)
    int defaultClearance = mmToUnits(0.2);
    auto clearanceMatrix = std::make_unique<ClearanceMatrix>(
      ClearanceMatrix::createDefault(kicadPcb.layers, defaultClearance));

    // Create routing board
    auto board = std::make_unique<RoutingBoard>(kicadPcb.layers, *clearanceMatrix);

    // Create nets collection and add to board
    // Note: We're leaking this for now - TODO: fix ownership model
    auto nets = new Nets();
    for (const auto& net : kicadPcb.nets) {
      nets->addNet(net);
    }
    board->setNets(nets);

    // Convert and add segments (traces)
    int itemId = 1;
    for (const auto& segment : kicadPcb.segments) {
      auto trace = convertSegmentToTrace(segment, itemId++, board.get());
      if (trace) {
        board->addItem(std::move(trace));
      }
    }

    // Convert and add vias
    for (const auto& via : kicadPcb.vias) {
      auto viaItem = convertViaToItem(via, itemId++, board.get());
      if (viaItem) {
        board->addItem(std::move(viaItem));
      }
    }

    // Convert and add footprint pads as pins
    int componentNumber = 1;
    for (const auto& footprint : kicadPcb.footprints) {
      convertFootprintPads(footprint, componentNumber++, itemId, board.get());
    }

    // Store footprints for visualization (courtyards, silkscreen)
    auto footprintsPtr = std::make_shared<std::vector<KiCadFootprint>>(kicadPcb.footprints);
    board->setFootprints(footprintsPtr);

    return {std::move(board), std::move(clearanceMatrix)};
  }

  // Convert RoutingBoard back to KiCadPcb (update segments and vias)
  static void updateKiCadPcbFromBoard(KiCadPcb& kicadPcb, const RoutingBoard& board) {
    // Clear existing segments and vias (we'll rebuild from board state)
    kicadPcb.segments.clear();
    kicadPcb.vias.clear();

    // Extract all traces and vias from the board
    for (const auto& itemPtr : board.getItems()) {
      const Item* item = itemPtr.get();
      if (!item) continue;

      // Check if it's a Trace
      if (const Trace* trace = dynamic_cast<const Trace*>(item)) {
        KiCadSegment segment;
        convertPointBack(trace->getStart(), segment.startX, segment.startY);
        convertPointBack(trace->getEnd(), segment.endX, segment.endY);
        segment.width = unitsToMm(trace->getWidth());
        segment.layer = trace->getLayer();
        segment.netNumber = trace->netCount() > 0 ? trace->getNets()[0] : 0;
        // UUID will be generated if needed
        kicadPcb.segments.push_back(segment);
      }
      // Check if it's a Via
      else if (const Via* via = dynamic_cast<const Via*>(item)) {
        KiCadVia kicadVia;
        convertPointBack(via->getCenter(), kicadVia.x, kicadVia.y);

        // Get via dimensions (simplified - padstack doesn't have shape API yet)
        // TODO: Extract actual dimensions from padstack when shape API is available
        kicadVia.size = 0.8;   // Default 0.8mm
        kicadVia.drill = 0.4;  // Default 0.4mm

        kicadVia.layersFrom = via->firstLayer();
        kicadVia.layersTo = via->lastLayer();
        kicadVia.netNumber = via->netCount() > 0 ? via->getNets()[0] : 0;
        kicadPcb.vias.push_back(kicadVia);
      }
    }
  }

private:
  // Convert KiCad segment to Trace
  static std::unique_ptr<Trace> convertSegmentToTrace(
      const KiCadSegment& segment, int itemId, BasicBoard* board) {

    IntPoint start = convertPoint(segment.startX, segment.startY);
    IntPoint end = convertPoint(segment.endX, segment.endY);
    int halfWidth = mmToUnits(segment.width) / 2;

    std::vector<int> nets;
    if (segment.netNumber > 0) {
      nets.push_back(segment.netNumber);
    }

    return std::make_unique<Trace>(
      start, end, segment.layer, halfWidth,
      nets, 0 /* clearanceClass */, itemId,
      FixedState::NotFixed, board
    );
  }

  // Convert KiCad via to Via
  static std::unique_ptr<Via> convertViaToItem(
      const KiCadVia& kicadVia, int itemId, BasicBoard* board) {

    IntPoint center = convertPoint(kicadVia.x, kicadVia.y);

    std::vector<int> nets;
    if (kicadVia.netNumber > 0) {
      nets.push_back(kicadVia.netNumber);
    }

    // Create a simple padstack for the via
    // Note: We allocate this on the heap and never delete it (memory leak)
    // TODO: Proper padstack lifetime management
    Padstack* padstack = new Padstack(
      "via_" + std::to_string(itemId),  // name
      itemId,                            // number
      kicadVia.layersFrom,              // fromLayer
      kicadVia.layersTo,                // toLayer
      true,                              // attachAllowed
      false                              // placedAbsolute
    );

    return std::make_unique<Via>(
      center, padstack, nets, 0 /* clearanceClass */, itemId,
      FixedState::SystemFixed, true /* attachAllowed */, board
    );
  }

  // Convert footprint pads to pins
  static void convertFootprintPads(const KiCadFootprint& footprint,
                                    int componentNumber, int& itemId,
                                    BasicBoard* board) {
    int pinNumber = 0;
    for (const auto& pad : footprint.pads) {
      auto pin = convertPadToPin(pad, footprint, componentNumber, pinNumber++, itemId++, board);
      if (pin) {
        board->addItem(std::move(pin));
      }
    }
  }

  // Convert KiCad pad to Pin
  static std::unique_ptr<Pin> convertPadToPin(const KiCadPad& pad,
                                                const KiCadFootprint& footprint,
                                                int componentNumber,
                                                int pinNumber, int itemId,
                                                BasicBoard* board) {
    // Calculate absolute position (footprint position + pad offset)
    IntPoint center = convertPoint(
      footprint.x + pad.x,
      footprint.y + pad.y
    );

    // Get net number
    std::vector<int> nets;
    if (pad.netNumber > 0) {
      nets.push_back(pad.netNumber);
    }

    // Create a simple padstack for the pad
    // Note: We allocate this on the heap and never delete it (memory leak)
    // TODO: Proper padstack lifetime management
    Padstack* padstack = new Padstack(
      "pad_" + std::to_string(itemId),  // name
      itemId,                            // number
      pad.layer,                         // fromLayer (for now assume single layer)
      pad.layer,                         // toLayer
      true,                              // attachAllowed
      false                              // placedAbsolute
    );

    return std::make_unique<Pin>(
      center, pinNumber, padstack, nets, 0 /* clearanceClass */, itemId,
      componentNumber, FixedState::SystemFixed, board
    );
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_KICADBOARDCONVERTER_H
