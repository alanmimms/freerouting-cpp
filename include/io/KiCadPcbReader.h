#ifndef FREEROUTING_IO_KICADPCBREADER_H
#define FREEROUTING_IO_KICADPCBREADER_H

#include "KiCadPcb.h"
#include "SExprParser.h"
#include "SExprLexer.h"
#include "rules/NetClass.h"
#include <optional>
#include <fstream>
#include <sstream>

namespace freerouting {

// Reader for KiCad .kicad_pcb files
// Parses S-expression format into KiCadPcb structure
class KiCadPcbReader {
public:
  // Read from file
  static std::optional<KiCadPcb> readFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
      return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return readFromString(buffer.str());
  }

  // Read from string
  static std::optional<KiCadPcb> readFromString(const std::string& content) {
    SExprLexer lexer(content);
    SExprParser parser(lexer);

    auto root = parser.parse();
    if (!root || !root->isListWithKeyword("kicad_pcb")) {
      return std::nullopt;
    }

    KiCadPcb pcb;
    parseKiCadPcb(*root, pcb);

    // Always return the PCB even if invalid - caller can check isValid()
    return pcb;
  }

private:
  // Parse top-level kicad_pcb node
  static bool parseKiCadPcb(const SExprNode& node, KiCadPcb& pcb) {
    if (!node.isList() || node.childCount() == 0) {
      return false;
    }

    // Skip first child (the "kicad_pcb" keyword)
    for (size_t i = 1; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() == 0) {
        continue;
      }

      const auto* keyword = child->getChild(0);
      if (!keyword->isAtom()) {
        continue;
      }

      const std::string& kw = keyword->asString();

      if (kw == "version") {
        parseVersion(*child, pcb.version);
      } else if (kw == "generator") {
        if (child->childCount() >= 2) {
          pcb.version.generator = child->getChild(1)->asString();
        }
      } else if (kw == "general") {
        parseGeneral(*child, pcb.general);
      } else if (kw == "paper") {
        if (child->childCount() >= 2) {
          pcb.paper = child->getChild(1)->asString();
        }
      } else if (kw == "layers") {
        parseLayers(*child, pcb.layers);
      } else if (kw == "setup") {
        parseSetup(*child, pcb.setup);
      } else if (kw == "net") {
        parseNet(*child, pcb);
      } else if (kw == "net_class") {
        parseNetClass(*child, pcb);
      } else if (kw == "segment") {
        parseSegment(*child, pcb);
      } else if (kw == "via") {
        parseVia(*child, pcb);
      } else if (kw == "footprint" || kw == "module") {
        parseFootprint(*child, pcb);
      }
    }

    return pcb.isValid();
  }

  // Parse version number
  static void parseVersion(const SExprNode& node, KiCadVersion& version) {
    if (node.childCount() >= 2) {
      version.version = static_cast<int>(node.getChild(1)->asInt());
    }
  }

  // Parse general section
  static void parseGeneral(const SExprNode& node, KiCadGeneral& general) {
    for (size_t i = 1; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() < 2) {
        continue;
      }

      const std::string& kw = child->getChild(0)->asString();
      if (kw == "thickness") {
        general.thickness = child->getChild(1)->asDouble();
      }
    }
  }

  // Parse layers section
  static void parseLayers(const SExprNode& node, LayerStructure& layers) {
    std::vector<Layer> layerList;

    for (size_t i = 1; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() < 3) {
        continue;
      }

      // Format: (0 "F.Cu" signal) or (31 "B.Cu" signal) or (32 "B.Adhes" user)
      // Layer number is stored but not used in Phase 3 (simplified mapping)
      // int layerNum = static_cast<int>(child->getChild(0)->asInt());
      std::string layerName = child->getChild(1)->asString();
      std::string layerType = child->getChild(2)->asString();

      bool isSignal = (layerType == "signal");

      // Create layer with name (std::string manages lifetime)
      layerList.push_back(Layer(std::move(layerName), isSignal));
    }

    layers = LayerStructure(std::move(layerList));
  }

  // Parse setup section
  static void parseSetup(const SExprNode& node, KiCadSetup& setup) {
    for (size_t i = 1; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() < 2) {
        continue;
      }

      const std::string& kw = child->getChild(0)->asString();
      if (kw == "pad_to_mask_clearance") {
        setup.padToMaskClearance = child->getChild(1)->asDouble();
      }
      // Other setup parameters can be added here
    }
  }

  // Parse net definition
  static void parseNet(const SExprNode& node, KiCadPcb& pcb) {
    if (node.childCount() < 3) {
      return;
    }

    // Format: (net 0 "") or (net 1 "GND")
    int netNumber = static_cast<int>(node.getChild(1)->asInt());
    std::string netName = node.getChild(2)->asString();

    // Create a simple Net (we'll need a default NetClass for now)
    // In full implementation, this would reference proper net classes
    Net net(netName, 1, netNumber, nullptr, false);
    pcb.nets.addNet(net);
  }

  // Parse net_class definition (basic parsing for Phase 3)
  static void parseNetClass(const SExprNode& node, KiCadPcb& pcb) {
    if (node.childCount() < 2) {
      return;
    }

    // Format: (net_class "Default" "Description" ...)
    std::string className = node.getChild(1)->asString();
    pcb.netClassNames.push_back(className);

    // Full net class parsing with clearance/width rules will be added
    // when we integrate with the routing engine
  }

  // Parse segment (trace) definition
  static void parseSegment(const SExprNode& node, KiCadPcb& pcb) {
    KiCadSegment segment;
    segment.layer = 0;
    segment.netNumber = 0;
    segment.width = 0.15;  // Default 0.15mm

    // Parse segment properties
    for (size_t i = 1; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() < 2) {
        continue;
      }

      const std::string& kw = child->getChild(0)->asString();

      if (kw == "start") {
        if (child->childCount() >= 3) {
          segment.startX = child->getChild(1)->asDouble();
          segment.startY = child->getChild(2)->asDouble();
        }
      } else if (kw == "end") {
        if (child->childCount() >= 3) {
          segment.endX = child->getChild(1)->asDouble();
          segment.endY = child->getChild(2)->asDouble();
        }
      } else if (kw == "width") {
        segment.width = child->getChild(1)->asDouble();
      } else if (kw == "layer") {
        std::string layerName = child->getChild(1)->asString();
        // Map layer name to layer number
        for (int j = 0; j < pcb.layers.count(); j++) {
          if (pcb.layers[j].name == layerName) {
            segment.layer = j;
            break;
          }
        }
      } else if (kw == "net") {
        segment.netNumber = static_cast<int>(child->getChild(1)->asInt());
      } else if (kw == "uuid" || kw == "tstamp") {
        segment.uuid = child->getChild(1)->asString();
      }
    }

    pcb.segments.push_back(segment);
  }

  // Parse via definition
  static void parseVia(const SExprNode& node, KiCadPcb& pcb) {
    KiCadVia via;
    via.x = via.y = 0.0;
    via.size = 0.8;      // Default 0.8mm
    via.drill = 0.4;     // Default 0.4mm
    via.layersFrom = 0;  // F.Cu
    via.layersTo = pcb.layers.count() - 1;  // B.Cu
    via.netNumber = 0;

    for (size_t i = 1; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() < 2) {
        continue;
      }

      const std::string& kw = child->getChild(0)->asString();

      if (kw == "at") {
        if (child->childCount() >= 3) {
          via.x = child->getChild(1)->asDouble();
          via.y = child->getChild(2)->asDouble();
        }
      } else if (kw == "size") {
        via.size = child->getChild(1)->asDouble();
      } else if (kw == "drill") {
        via.drill = child->getChild(1)->asDouble();
      } else if (kw == "layers") {
        // Format: (layers "F.Cu" "B.Cu")
        if (child->childCount() >= 3) {
          std::string layer1 = child->getChild(1)->asString();
          std::string layer2 = child->getChild(2)->asString();

          // Find layer indices
          for (int j = 0; j < pcb.layers.count(); j++) {
            if (pcb.layers[j].name == layer1) {
              via.layersFrom = j;
            }
            if (pcb.layers[j].name == layer2) {
              via.layersTo = j;
            }
          }
        }
      } else if (kw == "net") {
        via.netNumber = static_cast<int>(child->getChild(1)->asInt());
      } else if (kw == "uuid" || kw == "tstamp") {
        via.uuid = child->getChild(1)->asString();
      }
    }

    pcb.vias.push_back(via);
  }

  // Parse footprint/module definition (simplified)
  static void parseFootprint(const SExprNode& node, KiCadPcb& pcb) {
    KiCadFootprint footprint;
    footprint.x = footprint.y = footprint.rotation = 0.0;
    footprint.layer = 0;

    // First child after keyword is the footprint library reference
    if (node.childCount() >= 2 && node.getChild(1)->isAtom()) {
      footprint.reference = node.getChild(1)->asString();
    }

    for (size_t i = 2; i < node.childCount(); i++) {
      const auto* child = node.getChild(i);
      if (!child->isList() || child->childCount() < 2) {
        continue;
      }

      const std::string& kw = child->getChild(0)->asString();

      if (kw == "at") {
        if (child->childCount() >= 3) {
          footprint.x = child->getChild(1)->asDouble();
          footprint.y = child->getChild(2)->asDouble();
          if (child->childCount() >= 4) {
            footprint.rotation = child->getChild(3)->asDouble();
          }
        }
      } else if (kw == "layer") {
        std::string layerName = child->getChild(1)->asString();
        for (int j = 0; j < pcb.layers.count(); j++) {
          if (pcb.layers[j].name == layerName) {
            footprint.layer = j;
            break;
          }
        }
      } else if (kw == "fp_text" && child->childCount() >= 3) {
        std::string textType = child->getChild(1)->asString();
        std::string textValue = child->getChild(2)->asString();
        if (textType == "reference") {
          footprint.reference = textValue;
        } else if (textType == "value") {
          footprint.value = textValue;
        }
      } else if (kw == "uuid" || kw == "tstamp") {
        footprint.uuid = child->getChild(1)->asString();
      }
    }

    pcb.footprints.push_back(footprint);
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_KICADPCBREADER_H
