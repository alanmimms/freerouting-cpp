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

    // Footprints
    for (const auto& footprint : pcb.footprints) {
      writeFootprint(footprint, pcb.layers, out, level + 1);
    }

    // Segments
    for (const auto& segment : pcb.segments) {
      writeSegment(segment, pcb.layers, out, level + 1);
    }

    // Vias
    for (const auto& via : pcb.vias) {
      writeVia(via, pcb.layers, out, level + 1);
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

  // Write segment definition
  static void writeSegment(const KiCadSegment& segment, const LayerStructure& layers,
                           std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(segment";
    out << " (start " << segment.startX << " " << segment.startY << ")";
    out << " (end " << segment.endX << " " << segment.endY << ")";
    out << " (width " << segment.width << ")";

    // Get layer name
    if (segment.layer >= 0 && segment.layer < layers.count()) {
      out << " (layer \"" << layers[segment.layer].name << "\")";
    } else {
      out << " (layer \"F.Cu\")";  // Default fallback
    }

    out << " (net " << segment.netNumber << ")";

    if (!segment.uuid.empty()) {
      out << " (uuid \"" << segment.uuid << "\")";
    }

    out << ")\n";
  }

  // Write via definition
  static void writeVia(const KiCadVia& via, const LayerStructure& layers,
                       std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(via";
    out << " (at " << via.x << " " << via.y << ")";
    out << " (size " << via.size << ")";
    out << " (drill " << via.drill << ")";

    // Write layer range
    out << " (layers";
    if (via.layersFrom >= 0 && via.layersFrom < layers.count()) {
      out << " \"" << layers[via.layersFrom].name << "\"";
    } else {
      out << " \"F.Cu\"";
    }
    if (via.layersTo >= 0 && via.layersTo < layers.count()) {
      out << " \"" << layers[via.layersTo].name << "\"";
    } else {
      out << " \"B.Cu\"";
    }
    out << ")";

    out << " (net " << via.netNumber << ")";

    if (!via.uuid.empty()) {
      out << " (uuid \"" << via.uuid << "\")";
    }

    out << ")\n";
  }

  // Write footprint definition (simplified)
  static void writeFootprint(const KiCadFootprint& footprint, const LayerStructure& layers,
                              std::ostream& out, int level) {
    writeIndent(out, level);
    out << "(footprint \"" << footprint.reference << "\"";

    if (footprint.x != 0.0 || footprint.y != 0.0 || footprint.rotation != 0.0) {
      out << " (at " << footprint.x << " " << footprint.y;
      if (footprint.rotation != 0.0) {
        out << " " << footprint.rotation;
      }
      out << ")";
    }

    // Layer
    if (footprint.layer >= 0 && footprint.layer < layers.count()) {
      out << " (layer \"" << layers[footprint.layer].name << "\")";
    } else {
      out << " (layer \"F.Cu\")";
    }

    // Reference text
    if (!footprint.reference.empty()) {
      out << "\n";
      writeIndent(out, level + 1);
      out << "(fp_text reference \"" << footprint.reference << "\" (at 0 0) (layer \"F.SilkS\"))";
    }

    // Value text
    if (!footprint.value.empty()) {
      out << "\n";
      writeIndent(out, level + 1);
      out << "(fp_text value \"" << footprint.value << "\" (at 0 0) (layer \"F.Fab\"))";
    }

    // UUID
    if (!footprint.uuid.empty()) {
      out << "\n";
      writeIndent(out, level + 1);
      out << "(uuid \"" << footprint.uuid << "\")";
    }

    out << ")\n";
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_KICADPCBWRITER_H
