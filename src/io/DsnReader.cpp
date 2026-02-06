#include "io/DsnReader.h"
#include "io/SExprParser.h"
#include "io/SExprLexer.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cmath>

namespace freerouting {

std::optional<DsnDesign> DsnReader::readFromFile(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Failed to open DSN file: " << filename << std::endl;
    return std::nullopt;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  // Fix common KiCad DSN export issue: (string_quote ") should be (string_quote "\"")
  // This is a known issue where KiCad exports an unclosed string
  size_t pos = 0;
  while ((pos = content.find("(string_quote \")", pos)) != std::string::npos) {
    // Replace (string_quote ") with (string_quote "\"")
    content.replace(pos, 16, "(string_quote \"\\\"\")");
    pos += 19;
  }

  return readFromString(content);
}

std::optional<DsnDesign> DsnReader::readFromString(const std::string& content) {
  SExprLexer lexer(content);
  SExprParser parser(lexer);
  auto root = parser.parse();

  if (!root) {
    std::cerr << "Failed to parse DSN S-expression" << std::endl;
    return std::nullopt;
  }

  DsnDesign design;
  if (!parseDesign(*root, design)) {
    std::cerr << "Failed to parse DSN design" << std::endl;
    return std::nullopt;
  }

  return design;
}

bool DsnReader::parseDesign(const SExprNode& root, DsnDesign& design) {
  if (!root.isList() || root.childCount() < 2) {
    return false;
  }

  std::string keyword = parseString(*root.getChild(0));
  // Accept both "pcb" and "PCB" (case-insensitive)
  std::transform(keyword.begin(), keyword.end(), keyword.begin(), ::tolower);
  if (keyword != "pcb") {
    return false;
  }

  design.name = parseString(*root.getChild(1));

  for (size_t i = 2; i < root.childCount(); ++i) {
    const SExprNode& section = *root.getChild(i);
    if (!section.isList() || section.childCount() == 0) continue;

    std::string sectionName = parseString(*section.getChild(0));

    if (sectionName == "parser") {
      parseParser(section, design.parser);
    } else if (sectionName == "resolution") {
      parseResolution(section, design.resolution);
    } else if (sectionName == "unit") {
      if (section.childCount() >= 2) {
        design.unit = parseString(*section.getChild(1));
      }
    } else if (sectionName == "structure") {
      parseStructure(section, design.structure);
    } else if (sectionName == "library") {
      parseLibrary(section, design.library);
    } else if (sectionName == "network") {
      parseNetwork(section, design.network);
    } else if (sectionName == "placement") {
      parsePlacement(section, design.placement);
    } else if (sectionName == "wiring") {
      parseWiring(section, design.wiring);
    }
  }

  return true;
}

bool DsnReader::parseParser(const SExprNode& expr, DsnParser& parser) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "string_quote" && item.childCount() >= 2) {
      parser.stringQuote = parseString(*item.getChild(1));
    } else if (name == "space_in_quoted_tokens" && item.childCount() >= 2) {
      parser.spaceInQuotedTokens = (parseString(*item.getChild(1)) == "on");
    } else if (name == "host_cad" && item.childCount() >= 2) {
      parser.hostCad = parseString(*item.getChild(1));
    } else if (name == "host_version" && item.childCount() >= 2) {
      parser.hostVersion = parseString(*item.getChild(1));
    }
  }
  return true;
}

bool DsnReader::parseResolution(const SExprNode& expr, DsnResolution& resolution) {
  if (expr.childCount() >= 3) {
    resolution.unit = parseString(*expr.getChild(1));
    auto val = parseInt(*expr.getChild(2));
    if (val.has_value()) {
      resolution.value = *val;
    }
  }
  return true;
}

bool DsnReader::parseStructure(const SExprNode& expr, DsnStructure& structure) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "layer") {
      DsnLayer layer;
      if (parseLayer(item, layer)) {
        layer.index = static_cast<int>(structure.layers.size());
        structure.layers.push_back(layer);
      }
    } else if (name == "boundary") {
      parseBoundary(item, structure.boundary);
    } else if (name == "plane") {
      DsnPolygon plane;
      if (parsePlane(item, plane)) {
        structure.planes.push_back(plane);
      }
    } else if (name == "rule") {
      DsnRule rule;
      if (parseRule(item, rule)) {
        structure.rules.push_back(rule);
      }
    }
  }
  return true;
}

bool DsnReader::parseLibrary(const SExprNode& expr, DsnLibrary& library) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "padstack") {
      DsnPadstack padstack;
      if (parsePadstack(item, padstack)) {
        library.padstacks.push_back(padstack);
      }
    } else if (name == "image") {
      DsnImage image;
      if (parseImage(item, image)) {
        library.images.push_back(image);
      }
    }
  }
  return true;
}

bool DsnReader::parseNetwork(const SExprNode& expr, DsnNetwork& network) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "net") {
      DsnNet net;
      if (parseNet(item, net)) {
        network.nets.push_back(net);
      }
    } else if (name == "class") {
      DsnClass classObj;
      if (parseClass(item, classObj)) {
        network.classes.push_back(classObj);
      }
    }
  }
  return true;
}

bool DsnReader::parsePlacement(const SExprNode& expr, DsnPlacement& placement) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "component") {
      DsnComponent component;
      if (parseComponent(item, component)) {
        placement.components.push_back(component);
      }
    }
  }
  return true;
}

bool DsnReader::parseWiring(const SExprNode& expr, DsnWiring& wiring) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "wire") {
      DsnWire wire;
      if (parseWire(item, wire)) {
        wiring.wires.push_back(wire);
      }
    } else if (name == "via") {
      DsnVia via;
      if (parseVia(item, via)) {
        wiring.vias.push_back(via);
      }
    }
  }
  return true;
}

bool DsnReader::parseLayer(const SExprNode& expr, DsnLayer& layer) {
  if (expr.childCount() >= 2) {
    layer.name = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "type" && item.childCount() >= 2) {
      layer.type = parseString(*item.getChild(1));
    }
  }

  return true;
}

bool DsnReader::parseBoundary(const SExprNode& expr, std::vector<DsnPath>& boundary) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList()) continue;

    DsnPath path;
    if (parsePath(item, path)) {
      boundary.push_back(path);
    }
  }
  return true;
}

bool DsnReader::parsePlane(const SExprNode& expr, DsnPolygon& plane) {
  if (expr.childCount() >= 2) {
    plane.layer = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "polygon") {
      for (size_t j = 1; j + 1 < item.childCount(); j += 2) {
        auto pt = parsePoint(*item.getChild(j), *item.getChild(j + 1));
        if (pt.has_value()) {
          plane.points.push_back(*pt);
        }
      }
    }
  }

  return true;
}

bool DsnReader::parseRule(const SExprNode& expr, DsnRule& rule) {
  if (expr.childCount() < 2) return false;

  rule.type = parseString(*expr.getChild(0));

  for (size_t i = 1; i < expr.childCount(); ++i) {
    auto val = parseDouble(*expr.getChild(i));
    if (val.has_value()) {
      rule.values.push_back(*val);
    }
  }

  return true;
}

bool DsnReader::parsePadstack(const SExprNode& expr, DsnPadstack& padstack) {
  if (expr.childCount() >= 2) {
    padstack.name = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "shape") {
      DsnPadstackShape pShape;
      if (item.childCount() >= 2) {
        pShape.layerName = parseString(*item.getChild(1));
      }
      padstack.shapes.push_back(pShape);
    } else if (name == "attach" && item.childCount() >= 2) {
      padstack.rotate = (parseString(*item.getChild(1)) != "off");
    }
  }

  return true;
}

bool DsnReader::parseNet(const SExprNode& expr, DsnNet& net) {
  if (expr.childCount() >= 2) {
    net.name = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "pins") {
      for (size_t j = 1; j < item.childCount(); ++j) {
        net.pins.push_back(parseString(*item.getChild(j)));
      }
    }
  }

  return true;
}

bool DsnReader::parseClass(const SExprNode& expr, DsnClass& classObj) {
  if (expr.childCount() >= 2) {
    classObj.name = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (item.isList() && item.childCount() >= 1) {
      std::string name = parseString(*item.getChild(0));
      if (name == "net") {
        for (size_t j = 1; j < item.childCount(); ++j) {
          classObj.netNames.push_back(parseString(*item.getChild(j)));
        }
      } else if (name == "rule") {
        DsnRule rule;
        if (parseRule(item, rule)) {
          classObj.rules.push_back(rule);
        }
      }
    }
  }

  return true;
}

bool DsnReader::parseComponent(const SExprNode& expr, DsnComponent& component) {
  // Format: (component "image_name" (place ...))
  if (expr.childCount() >= 2) {
    component.imageName = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "place") {
      parsePlace(item, component);
    } else if (name == "locked" && item.childCount() >= 2) {
      component.locked = (parseString(*item.getChild(1)) == "true");
    }
  }

  return true;
}

bool DsnReader::parseWire(const SExprNode& expr, DsnWire& wire) {
  for (size_t i = 1; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "path") {
      DsnPath path;
      if (parsePath(item, path)) {
        wire.paths.push_back(path);
      }
    } else if (name == "net" && item.childCount() >= 2) {
      wire.netName = parseString(*item.getChild(1));
    } else if (name == "type" && item.childCount() >= 2) {
      wire.type = parseString(*item.getChild(1));
    }
  }

  return true;
}

bool DsnReader::parseVia(const SExprNode& expr, DsnVia& via) {
  if (expr.childCount() >= 2) {
    via.padstackName = parseString(*expr.getChild(1));
  }

  if (expr.childCount() >= 4) {
    auto x = parseInt(*expr.getChild(2));
    auto y = parseInt(*expr.getChild(3));
    if (x.has_value() && y.has_value()) {
      via.position = IntPoint(*x, *y);
    }
  }

  for (size_t i = 4; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "net" && item.childCount() >= 2) {
      via.netName = parseString(*item.getChild(1));
    }
  }

  return true;
}

bool DsnReader::parsePath(const SExprNode& expr, DsnPath& path) {
  if (!expr.isList() || expr.childCount() == 0) return false;

  std::string keyword = parseString(*expr.getChild(0));
  if (keyword != "path" && keyword != "polygon") return false;

  if (expr.childCount() >= 2) {
    path.layer = parseString(*expr.getChild(1));
  }

  size_t startIdx = 2;
  if (expr.childCount() >= 3) {
    auto w = parseInt(*expr.getChild(2));
    if (w.has_value()) {
      path.width = *w;
      startIdx = 3;
    }
  }

  for (size_t i = startIdx; i + 1 < expr.childCount(); i += 2) {
    auto pt = parsePoint(*expr.getChild(i), *expr.getChild(i + 1));
    if (pt.has_value()) {
      path.points.push_back(*pt);
    }
  }

  return true;
}

bool DsnReader::parseRect(const SExprNode& expr, DsnRect& rect) {
  if (expr.childCount() >= 6) {
    rect.layer = parseString(*expr.getChild(1));

    auto x1 = parseInt(*expr.getChild(2));
    auto y1 = parseInt(*expr.getChild(3));
    auto x2 = parseInt(*expr.getChild(4));
    auto y2 = parseInt(*expr.getChild(5));

    if (x1 && y1 && x2 && y2) {
      rect.ll = IntPoint(*x1, *y1);
      rect.ur = IntPoint(*x2, *y2);
      return true;
    }
  }
  return false;
}

bool DsnReader::parseCircle(const SExprNode& expr, DsnCircle& circle) {
  if (expr.childCount() >= 5) {
    circle.layer = parseString(*expr.getChild(1));

    auto x = parseInt(*expr.getChild(2));
    auto y = parseInt(*expr.getChild(3));
    auto r = parseInt(*expr.getChild(4));

    if (x && y && r) {
      circle.center = IntPoint(*x, *y);
      circle.radius = *r;
      return true;
    }
  }
  return false;
}

bool DsnReader::parsePolygon(const SExprNode& expr, DsnPolygon& polygon) {
  if (expr.childCount() >= 2) {
    polygon.layer = parseString(*expr.getChild(1));
  }

  for (size_t i = 2; i + 1 < expr.childCount(); i += 2) {
    auto pt = parsePoint(*expr.getChild(i), *expr.getChild(i + 1));
    if (pt.has_value()) {
      polygon.points.push_back(*pt);
    }
  }

  return true;
}

bool DsnReader::parseShape(const SExprNode& expr, DsnShape& shape) {
  if (!expr.isList() || expr.childCount() == 0) return false;

  std::string keyword = parseString(*expr.getChild(0));

  if (keyword == "path") {
    DsnPath path;
    if (parsePath(expr, path)) {
      shape.type = DsnShape::Type::Path;
      shape.path = path;
      return true;
    }
  } else if (keyword == "rect") {
    DsnRect rect;
    if (parseRect(expr, rect)) {
      shape.type = DsnShape::Type::Rect;
      shape.rect = rect;
      return true;
    }
  } else if (keyword == "circle") {
    DsnCircle circle;
    if (parseCircle(expr, circle)) {
      shape.type = DsnShape::Type::Circle;
      shape.circle = circle;
      return true;
    }
  } else if (keyword == "polygon") {
    DsnPolygon polygon;
    if (parsePolygon(expr, polygon)) {
      shape.type = DsnShape::Type::Polygon;
      shape.polygon = polygon;
      return true;
    }
  }

  return false;
}

std::optional<IntPoint> DsnReader::parsePoint(const SExprNode& x, const SExprNode& y) {
  auto xVal = parseInt(x);
  auto yVal = parseInt(y);

  if (xVal.has_value() && yVal.has_value()) {
    return IntPoint(*xVal, *yVal);
  }

  return std::nullopt;
}

std::string DsnReader::parseString(const SExprNode& expr) {
  if (!expr.isAtom()) return "";

  const auto& value = expr.getValue();
  if (std::holds_alternative<std::string>(value)) {
    return std::get<std::string>(value);
  } else if (std::holds_alternative<i64>(value)) {
    return std::to_string(std::get<i64>(value));
  } else if (std::holds_alternative<double>(value)) {
    return std::to_string(std::get<double>(value));
  }

  return "";
}

std::optional<int> DsnReader::parseInt(const SExprNode& expr) {
  if (!expr.isAtom()) return std::nullopt;

  const auto& value = expr.getValue();
  if (std::holds_alternative<i64>(value)) {
    return static_cast<int>(std::get<i64>(value));
  } else if (std::holds_alternative<double>(value)) {
    return static_cast<int>(std::get<double>(value));
  }

  return std::nullopt;
}

std::optional<double> DsnReader::parseDouble(const SExprNode& expr) {
  if (!expr.isAtom()) return std::nullopt;

  const auto& value = expr.getValue();
  if (std::holds_alternative<double>(value)) {
    return std::get<double>(value);
  } else if (std::holds_alternative<i64>(value)) {
    return static_cast<double>(std::get<i64>(value));
  }

  return std::nullopt;
}

bool DsnReader::parseImage(const SExprNode& expr, DsnImage& image) {
  if (expr.childCount() >= 2) {
    image.name = parseString(*expr.getChild(1));
  }

  int pinCount = 0;
  for (size_t i = 2; i < expr.childCount(); ++i) {
    const SExprNode& item = *expr.getChild(i);
    if (!item.isList() || item.childCount() == 0) continue;

    std::string name = parseString(*item.getChild(0));
    if (name == "side" && item.childCount() >= 2) {
      image.side = parseString(*item.getChild(1));
    } else if (name == "pin") {
      DsnImagePin pin;
      if (parseImagePin(item, pin)) {
        image.pins.push_back(pin);
        pinCount++;
      }
    }
  }


  return true;
}

bool DsnReader::parseImagePin(const SExprNode& expr, DsnImagePin& pin) {
  // Format: (pin "padstack_name" pin_number x y)
  if (expr.childCount() >= 5) {
    pin.padstackName = parseString(*expr.getChild(1));
    pin.pinNumber = parseString(*expr.getChild(2));

    auto x = parseInt(*expr.getChild(3));
    auto y = parseInt(*expr.getChild(4));
    if (x && y) {
      pin.position = IntPoint(*x, *y);
      return true;
    }
  }
  return false;
}

bool DsnReader::parsePlace(const SExprNode& expr, DsnComponent& component) {
  // Format: (place refdes x y side rotation (pin ...) (pin ...) ...)
  if (expr.childCount() >= 5) {
    component.refdes = parseString(*expr.getChild(1));

    auto x = parseInt(*expr.getChild(2));
    auto y = parseInt(*expr.getChild(3));
    if (x && y) {
      component.position = IntPoint(*x, *y);
    }

    std::string side = parseString(*expr.getChild(4));
    component.onFront = (side == "front");

    if (expr.childCount() >= 6) {
      auto rot = parseDouble(*expr.getChild(5));
      if (rot.has_value()) {
        component.rotation = *rot;
      }
    }

    // Parse pin clearance overrides
    for (size_t i = 6; i < expr.childCount(); ++i) {
      const SExprNode& item = *expr.getChild(i);
      if (item.isList() && item.childCount() > 0) {
        std::string name = parseString(*item.getChild(0));
        if (name == "pin") {
          DsnComponentPin pin;
          if (parseComponentPin(item, pin)) {
            component.pins.push_back(pin);
          }
        }
      }
    }
  }

  return true;
}

bool DsnReader::parseComponentPin(const SExprNode& expr, DsnComponentPin& pin) {
  // Format: (pin number (clearance_class class_name))
  if (expr.childCount() >= 2) {
    pin.number = parseString(*expr.getChild(1));

    for (size_t i = 2; i < expr.childCount(); ++i) {
      const SExprNode& item = *expr.getChild(i);
      if (item.isList() && item.childCount() >= 2) {
        std::string name = parseString(*item.getChild(0));
        if (name == "clearance_class") {
          pin.clearanceClass = parseString(*item.getChild(1));
        }
      }
    }
    return true;
  }
  return false;
}

const SExprNode* DsnReader::findChild(const SExprNode& expr, const std::string& name) {
  if (!expr.isList()) return nullptr;

  for (size_t i = 0; i < expr.childCount(); ++i) {
    const SExprNode* child = expr.getChild(i);
    if (child->isList() && child->childCount() > 0) {
      if (parseString(*child->getChild(0)) == name) {
        return child;
      }
    }
  }

  return nullptr;
}

std::vector<const SExprNode*> DsnReader::findChildren(const SExprNode& expr, const std::string& name) {
  std::vector<const SExprNode*> result;

  if (!expr.isList()) return result;

  for (size_t i = 0; i < expr.childCount(); ++i) {
    const SExprNode* child = expr.getChild(i);
    if (child->isList() && child->childCount() > 0) {
      if (parseString(*child->getChild(0)) == name) {
        result.push_back(child);
      }
    }
  }

  return result;
}

} // namespace freerouting
