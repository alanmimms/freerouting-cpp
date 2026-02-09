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

// Trace segment from KiCad PCB file
struct KiCadSegment {
  double startX, startY;
  double endX, endY;
  double width;
  int layer;
  int netNumber;
  std::string uuid;
};

// Via from KiCad PCB file
struct KiCadVia {
  double x, y;
  double size;         // Outer diameter
  double drill;        // Drill diameter
  int layersFrom, layersTo;
  int netNumber;
  std::string uuid;
};

// Pad from a footprint
struct KiCadPad {
  std::string padNumber;  // Pin number/name (can be "1", "2", "A1", etc.)
  std::string type;        // thru_hole, smd, connect, np_thru_hole
  std::string shape;       // circle, rect, oval, roundrect, etc.
  double x, y;            // Position relative to footprint origin
  double sizeX, sizeY;    // Pad dimensions
  double drill;           // Drill diameter (0 for SMD)
  int layer;              // Primary layer (0=F.Cu, 31=B.Cu, etc.)
  int netNumber;          // Net this pad belongs to
  std::string netName;    // Net name
};

// Graphical line in a footprint (fp_line)
struct KiCadFpLine {
  double startX, startY;
  double endX, endY;
  std::string layer;      // "F.SilkS", "B.SilkS", "F.CrtYd", "B.CrtYd", "F.Fab", etc.
  double width;
};

// Graphical text in a footprint (fp_text)
struct KiCadFpText {
  std::string type;       // "reference", "value", "user"
  std::string text;
  double x, y;
  std::string layer;
  double fontSize;
  double thickness;
  double rotation;
};

// Footprint/module from KiCad PCB file
struct KiCadFootprint {
  std::string reference;
  std::string value;
  double x, y, rotation;
  int layer;
  std::string uuid;
  std::vector<KiCadPad> pads;          // Pads within this footprint
  std::vector<KiCadFpLine> fpLines;    // Graphical lines (silkscreen, courtyard, fab)
  std::vector<KiCadFpText> fpTexts;    // Text elements
};

// Complete KiCad PCB file representation
// Extended to include board items for routing
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

  // Board items
  std::vector<KiCadSegment> segments;
  std::vector<KiCadVia> vias;
  std::vector<KiCadFootprint> footprints;

  // Board outline coordinates (simplified)
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
