#ifndef FREEROUTING_IO_DSNBOARDCONVERTER_H
#define FREEROUTING_IO_DSNBOARDCONVERTER_H

#include "io/DsnStructs.h"
#include "board/RoutingBoard.h"
#include "rules/ClearanceMatrix.h"
#include <memory>
#include <utility>

namespace freerouting {

// Converts DSN design to RoutingBoard for autorouting
class DsnBoardConverter {
public:
  // Convert DSN design to RoutingBoard
  // Returns pair of (board, clearanceMatrix)
  static std::pair<std::unique_ptr<RoutingBoard>, ClearanceMatrix>
  createRoutingBoard(const DsnDesign& dsn);

private:
  // Helper: create layer structure from DSN layers
  static LayerStructure createLayerStructure(const DsnDesign& dsn);

  // Helper: create clearance matrix from DSN rules
  static ClearanceMatrix createClearanceMatrix(const DsnDesign& dsn,
                                                 const LayerStructure& layers);

  // Helper: add existing traces/vias to board
  static void addWiring(RoutingBoard* board, const DsnDesign& dsn);

  // Helper: add components and pins to board
  static void addComponents(RoutingBoard* board, const DsnDesign& dsn);
};

} // namespace freerouting

#endif // FREEROUTING_IO_DSNBOARDCONVERTER_H
