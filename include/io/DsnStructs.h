#ifndef FREEROUTING_IO_DSNSTRUCTS_H
#define FREEROUTING_IO_DSNSTRUCTS_H

#include "core/Types.h"
#include "geometry/Vector2.h"
#include <string>
#include <vector>
#include <map>
#include <optional>

namespace freerouting {

// Specctra DSN (Design) file format structures
// Based on Specctra specification and KiCad's DSN export

// Parser settings
struct DsnParser {
  std::string stringQuote = "\"";
  bool spaceInQuotedTokens = true;
  std::string hostCad;
  std::string hostVersion;
};

// Resolution and units
struct DsnResolution {
  std::string unit = "um";  // um, mil, inch, mm
  int value = 10;           // units per specified unit (e.g., 10 = 0.1um resolution)
};

// Layer definition
struct DsnLayer {
  std::string name;
  std::string type;  // signal, power, jumper, etc.
  int index = -1;
  std::map<std::string, std::string> properties;
};

// Geometric path (for boundaries, polygons, etc.)
struct DsnPath {
  std::string layer;
  int width = 0;
  std::vector<IntPoint> points;
};

// Rectangle
struct DsnRect {
  std::string layer;
  IntPoint ll;  // lower-left
  IntPoint ur;  // upper-right
};

// Circle
struct DsnCircle {
  std::string layer;
  IntPoint center;
  int radius = 0;
};

// Polygon (for planes, keepouts, etc.)
struct DsnPolygon {
  std::string layer;
  std::vector<IntPoint> points;
  std::vector<DsnPath> cutouts;  // window cutouts in planes
};

// Shape (can be path, rect, circle, polygon)
struct DsnShape {
  enum class Type { Path, Rect, Circle, Polygon };
  Type type;

  std::optional<DsnPath> path;
  std::optional<DsnRect> rect;
  std::optional<DsnCircle> circle;
  std::optional<DsnPolygon> polygon;
};

// Padstack shape on a layer
struct DsnPadstackShape {
  std::string layerName;  // Layer name or range like "F.Cu-B.Cu"
  std::vector<DsnShape> shapes;
};

// Padstack definition (via or pad)
struct DsnPadstack {
  std::string name;
  std::vector<DsnPadstackShape> shapes;
  bool rotate = true;
  bool absolute = false;

  // Derived properties
  int fromLayerIndex = -1;
  int toLayerIndex = -1;
};

// Design rule
struct DsnRule {
  std::string type;  // width, clearance, etc.
  std::vector<double> values;
};

// Clearance class
struct DsnClass {
  std::string name;
  std::vector<std::string> netNames;
  std::vector<DsnRule> rules;
  int clearanceValue = 0;
  std::string circuitType;  // FIX, ROUTE, etc.
};

// Net definition
struct DsnNet {
  std::string name;
  std::vector<std::string> pins;  // Component-pin references like "U1-1"
};

// Pin in footprint image (relative position)
struct DsnImagePin {
  std::string padstackName;  // Padstack type (e.g., "Round[A]Pad_1700_um")
  std::string pinNumber;     // Pin number/name (e.g., "1", "2")
  IntPoint position;         // Relative position in footprint
};

// Footprint image definition
struct DsnImage {
  std::string name;                   // Image/footprint name
  std::string side = "front";         // Front or back
  std::vector<DsnImagePin> pins;      // Pin definitions
};

// Pin clearance info in component placement
struct DsnComponentPin {
  std::string number;
  std::string clearanceClass;
};

// Component instance
struct DsnComponent {
  std::string refdes;      // Reference designator (e.g., "U1")
  std::string imageName;   // Footprint/image name
  IntPoint position;
  double rotation = 0.0;   // degrees
  bool onFront = true;
  bool locked = false;
  std::vector<DsnComponentPin> pins;  // Pin clearance overrides
};

// Via instance
struct DsnVia {
  std::string padstackName;
  IntPoint position;
  std::string netName;
};

// Wire segment
struct DsnWire {
  std::string netName;
  std::vector<DsnPath> paths;
  std::string type;  // protect, fix, route, etc.
};

// Board structure section
struct DsnStructure {
  std::vector<DsnLayer> layers;
  std::vector<DsnPath> boundary;
  std::vector<DsnPolygon> planes;
  std::vector<DsnRule> rules;
  std::vector<DsnShape> keepouts;
  std::map<std::string, std::string> properties;
};

// Library section (component definitions and padstacks)
struct DsnLibrary {
  std::vector<DsnPadstack> padstacks;
  std::vector<DsnImage> images;  // Footprint/image definitions
};

// Network section (nets and classes)
struct DsnNetwork {
  std::vector<DsnNet> nets;
  std::vector<DsnClass> classes;
};

// Placement section (component positions)
struct DsnPlacement {
  std::vector<DsnComponent> components;
};

// Wiring section (existing routes)
struct DsnWiring {
  std::vector<DsnWire> wires;
  std::vector<DsnVia> vias;
};

// Complete DSN design
struct DsnDesign {
  std::string name;
  DsnParser parser;
  DsnResolution resolution;
  std::string unit = "um";

  DsnStructure structure;
  DsnLibrary library;
  DsnNetwork network;
  DsnPlacement placement;
  DsnWiring wiring;

  // Helper methods
  const DsnLayer* findLayer(const std::string& name) const {
    for (const auto& layer : structure.layers) {
      if (layer.name == name) return &layer;
    }
    return nullptr;
  }

  const DsnPadstack* findPadstack(const std::string& name) const {
    for (const auto& ps : library.padstacks) {
      if (ps.name == name) return &ps;
    }
    return nullptr;
  }

  const DsnImage* findImage(const std::string& name) const {
    for (const auto& img : library.images) {
      if (img.name == name) return &img;
    }
    return nullptr;
  }

  const DsnNet* findNet(const std::string& name) const {
    for (const auto& net : network.nets) {
      if (net.name == name) return &net;
    }
    return nullptr;
  }

  int getLayerIndex(const std::string& name) const {
    for (const auto& layer : structure.layers) {
      if (layer.name == name) return layer.index;
    }
    return -1;
  }

  // Convert DSN units to internal units (1mm = 10000 internal units)
  int toInternalUnits(int dsnValue) const {
    // resolution.value is units per resolution.unit
    // e.g., resolution um 10 means 10 units = 1 um = 0.001 mm
    // So 1 DSN unit = (0.001 mm / 10) = 0.0001 mm = 0.1 um
    // Internal: 1 mm = 10000 units
    // So 1 DSN unit = 0.0001 mm * 10000 = 1 internal unit

    if (unit == "um") {
      // 1 um = 0.001 mm = 10 internal units
      // DSN value is in (resolution.value) per um
      // So DSN value / resolution.value = um
      // um * 10 = internal units
      return (dsnValue * 10) / resolution.value;
    } else if (unit == "mm") {
      // 1 mm = 10000 internal units
      return (dsnValue * 10000) / resolution.value;
    } else if (unit == "mil") {
      // 1 mil = 0.0254 mm = 254 internal units
      return (dsnValue * 254) / resolution.value;
    } else if (unit == "inch") {
      // 1 inch = 25.4 mm = 254000 internal units
      return (dsnValue * 254000) / resolution.value;
    }

    // Default: assume um
    return (dsnValue * 10) / resolution.value;
  }
};

} // namespace freerouting

#endif // FREEROUTING_IO_DSNSTRUCTS_H
