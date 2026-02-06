#ifndef FREEROUTING_IO_KICADPCBWRITER_H
#define FREEROUTING_IO_KICADPCBWRITER_H

#include "KiCadPcb.h"
#include <sstream>
#include <fstream>
#include <iomanip>

namespace freerouting {

// Writer for KiCad .kicad_pcb files
// Outputs KiCadPcb structure as S-expression format
class KiCadPcbWriter {
public:
  // Write to file
  static bool writeToFile(const KiCadPcb& pcb, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      return false;
    }

    std::string content = writeToString(pcb);
    file << content;
    return file.good();
  }

  // Write to string
  static std::string writeToString(const KiCadPcb& pcb) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6);

    writeKiCadPcb(pcb, out, 0);

    return out.str();
  }

private:
  // Write indentation
  static void writeIndent(std::ostream& out, int level) {
    for (int i = 0; i < level; i++) {
      out << "  ";
    }
  }

  // Write top-level kicad_pcb structure
  static void writeKiCadPcb(const KiCadPcb& pcb, std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(kicad_pcb";

    // Version
    out << " (version " << pcb.version.version << ")";

    // Generator
    out << " (generator " << pcb.version.generator << ")\n";

    // General section
    writeGeneral(pcb.general, out, level + 1);

    // Paper
    writeIndent(out, level + 1);
    out << "(paper \"" << pcb.paper << "\")\n";

    // Layers
    writeLayers(pcb.layers, out, level + 1);

    // Setup
    writeSetup(pcb.setup, out, level + 1);

    // Nets
    for (const auto& net : pcb.nets) {
      writeNet(net, out, level + 1);
    }

    // Net classes (simplified for Phase 3)
    for (const auto& netClassName : pcb.netClassNames) {
      writeIndent(out, level + 1);
      out << "(net_class \"" << netClassName << "\" \"\")\n";
    }

    writeIndent(out, level);
    out << ")\n";
  }

  // Write general section
  static void writeGeneral(const KiCadGeneral& general, std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(general\n";
    writeIndent(out, level + 1);
    out << "(thickness " << general.thickness << "))\n";
  }

  // Write layers section
  static void writeLayers(const LayerStructure& layers, std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(layers\n";

    // Map layer names to KiCad layer numbers
    for (int i = 0; i < layers.count(); i++) {
      const Layer& layer = layers[i];
      writeIndent(out, level + 1);

      // Simplified layer numbering for Phase 3
      int layerNum = i;
      if (layer.name == "F.Cu") layerNum = 0;
      else if (layer.name == "B.Cu") layerNum = 31;

      out << "(" << layerNum << " \"" << layer.name << "\" ";
      out << (layer.isSignal ? "signal" : "user") << ")\n";
    }

    writeIndent(out, level);
    out << ")\n";
  }

  // Write setup section
  static void writeSetup(const KiCadSetup& setup, std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(setup\n";
    writeIndent(out, level + 1);
    out << "(pad_to_mask_clearance " << setup.padToMaskClearance << ")\n";
    writeIndent(out, level);
    out << ")\n";
  }

  // Write net definition
  static void writeNet(const Net& net, std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(net " << net.getNetNumber() << " \"" << net.getName() << "\")\n";
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_KICADPCBWRITER_H
