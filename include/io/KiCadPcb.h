#ifndef FREEROUTING_IO_KICADPCB_H
#define FREEROUTING_IO_KICADPCB_H

#include "board/Layer.h"
#include "board/LayerStructure.h"
#include "rules/Net.h"
#include "rules/Nets.h"
#include "rules/NetClass.h"
#include "rules/ClearanceMatrix.h"
#include <string>
#include <memory>
#include <optional>

namespace freerouting {

// KiCad PCB file version information
struct KiCadVersion {
  int version;         // e.g., 20221018
  std::string generator; // e.g., "pcbnew"

  KiCadVersion() : version(20221018), generator("pcbnew") {}
  KiCadVersion(int v, std::string gen) : version(v), generator(std::move(gen)) {}
};

// General board properties
struct KiCadGeneral {
  double thickness;    // Board thickness in mm

  KiCadGeneral() : thickness(1.6) {}
};

// Board setup/design rules
struct KiCadSetup {
  double padToMaskClearance;
  double solder_mask_min_width;
  double solder_paste_margin;
  double solder_paste_ratio;

  KiCadSetup()
    : padToMaskClearance(0.0),
      solder_mask_min_width(0.0),
      solder_paste_margin(0.0),
      solder_paste_ratio(-0.0) {}
};

// Complete KiCad PCB file representation
// This is Phase 3 version - basic metadata only
// Full board items (segments, vias, pads, etc.) will be added in later phases
class KiCadPcb {
public:
  KiCadPcb() = default;

  // Version information
  KiCadVersion version;

  // General board properties
  KiCadGeneral general;

  // Paper size (e.g., "A4", "User")
  std::string paper;

  // Layer structure
  LayerStructure layers;

  // Setup/design rules
  KiCadSetup setup;

  // Nets collection
  Nets nets;

  // Net classes (not yet fully implemented - stored as raw data for now)
  // Full NetClass integration requires clearance matrix setup
  std::vector<std::string> netClassNames;

  // Board outline coordinates (simplified for Phase 3)
  // Full polygon support in later phases
  struct BoardOutline {
    double minX, minY, maxX, maxY;
    BoardOutline() : minX(0), minY(0), maxX(100), maxY(100) {}
  };
  BoardOutline outline;

  // Get layer by KiCad layer number
  const Layer* getLayer(int kiCadLayerNumber) const {
    for (int i = 0; i < layers.count(); i++) {
      // In KiCad, F.Cu is typically 0, B.Cu is 31
      // We'll store a mapping as needed
      // For now, simplified direct indexing
      if (i == kiCadLayerNumber ||
          (kiCadLayerNumber == 0 && layers[i].name == "F.Cu") ||
          (kiCadLayerNumber == 31 && layers[i].name == "B.Cu")) {
        return &layers[i];
      }
    }
    return nullptr;
  }

  // Get net by number
  const Net* getNet(int netNumber) const {
    return nets.getNet(netNumber);
  }

  // Get net by name
  const Net* getNet(const std::string& netName) const {
    return nets.getNet(netName);
  }

  // Validate PCB structure
  bool isValid() const {
    return layers.count() > 0 && !paper.empty();
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_KICADPCB_H
