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
      }
      // Other keywords (segment, via, module, etc.) will be handled in later phases
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
};

} // namespace freerouting

#endif // FREEROUTING_IO_KICADPCBREADER_H
