#ifndef FREEROUTING_IO_DSNREADER_H
#define FREEROUTING_IO_DSNREADER_H

#include "io/DsnStructs.h"
#include "io/SExprParser.h"
#include <string>
#include <optional>
#include <memory>

namespace freerouting {

// Reader for Specctra DSN (Design) files
// Parses S-expression based DSN format into DsnDesign structure
class DsnReader {
public:
  // Read DSN file and return design structure
  static std::optional<DsnDesign> readFromFile(const std::string& filename);

  // Read DSN from string content
  static std::optional<DsnDesign> readFromString(const std::string& content);

private:
  // Parse DSN design from S-expression
  static bool parseDesign(const SExprNode& root, DsnDesign& design);

  // Parse individual sections
  static bool parseParser(const SExprNode& expr, DsnParser& parser);
  static bool parseResolution(const SExprNode& expr, DsnResolution& resolution);
  static bool parseStructure(const SExprNode& expr, DsnStructure& structure);
  static bool parseLibrary(const SExprNode& expr, DsnLibrary& library);
  static bool parseNetwork(const SExprNode& expr, DsnNetwork& network);
  static bool parsePlacement(const SExprNode& expr, DsnPlacement& placement);
  static bool parseWiring(const SExprNode& expr, DsnWiring& wiring);

  // Parse structure subsections
  static bool parseLayer(const SExprNode& expr, DsnLayer& layer);
  static bool parseBoundary(const SExprNode& expr, std::vector<DsnPath>& boundary);
  static bool parsePlane(const SExprNode& expr, DsnPolygon& plane);
  static bool parseRule(const SExprNode& expr, DsnRule& rule);

  // Parse library subsections
  static bool parsePadstack(const SExprNode& expr, DsnPadstack& padstack);
  static bool parseImage(const SExprNode& expr, DsnImage& image);
  static bool parseImagePin(const SExprNode& expr, DsnImagePin& pin);

  // Parse network subsections
  static bool parseNet(const SExprNode& expr, DsnNet& net);
  static bool parseClass(const SExprNode& expr, DsnClass& classObj);

  // Parse placement subsections
  static bool parseComponent(const SExprNode& expr, DsnComponent& component);
  static bool parsePlace(const SExprNode& expr, DsnComponent& component);
  static bool parseComponentPin(const SExprNode& expr, DsnComponentPin& pin);

  // Parse wiring subsections
  static bool parseWire(const SExprNode& expr, DsnWire& wire);
  static bool parseVia(const SExprNode& expr, DsnVia& via);

  // Parse geometric primitives
  static bool parsePath(const SExprNode& expr, DsnPath& path);
  static bool parseRect(const SExprNode& expr, DsnRect& rect);
  static bool parseCircle(const SExprNode& expr, DsnCircle& circle);
  static bool parsePolygon(const SExprNode& expr, DsnPolygon& polygon);
  static bool parseShape(const SExprNode& expr, DsnShape& shape);

  // Helper: parse point coordinates
  static std::optional<IntPoint> parsePoint(const SExprNode& x, const SExprNode& y);

  // Helper: parse string from atom
  static std::string parseString(const SExprNode& expr);

  // Helper: parse integer from atom
  static std::optional<int> parseInt(const SExprNode& expr);

  // Helper: parse double from atom
  static std::optional<double> parseDouble(const SExprNode& expr);

  // Helper: find child by name
  static const SExprNode* findChild(const SExprNode& expr, const std::string& name);

  // Helper: get all children with name
  static std::vector<const SExprNode*> findChildren(const SExprNode& expr, const std::string& name);
};

} // namespace freerouting

#endif // FREEROUTING_IO_DSNREADER_H
