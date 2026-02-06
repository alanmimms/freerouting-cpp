#include <catch2/catch_test_macros.hpp>
#include "geometry/PolyLine.h"
#include "geometry/ComplexPolygon.h"
#include "board/BoardOutline.h"
#include "board/RuleArea.h"
#include "board/ConductionArea.h"

using namespace freerouting;

// ============================================================================
// PolyLine Tests
// ============================================================================

TEST_CASE("PolyLine - Open polyline", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100)
  };

  PolyLine polyline(points, false);  // Open

  REQUIRE(polyline.pointCount() == 3);
  REQUIRE(!polyline.isClosed());
  REQUIRE(polyline.isEmpty() == false);
  REQUIRE(polyline.dimension() == 1);
  REQUIRE(polyline.area() == 0.0);  // Open polylines have no area
}

TEST_CASE("PolyLine - Closed polyline", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  PolyLine polyline(points, true);  // Closed

  REQUIRE(polyline.pointCount() == 4);
  REQUIRE(polyline.isClosed());
  REQUIRE(polyline.dimension() == 2);
  REQUIRE(polyline.area() > 0.0);  // Closed polylines have area
  REQUIRE(polyline.area() == 10000.0);  // 100x100 square
}

TEST_CASE("PolyLine - Bounding box", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(10, 20),
    IntPoint(50, 80),
    IntPoint(30, 60)
  };

  PolyLine polyline(points);

  IntBox bbox = polyline.getBoundingBox();
  REQUIRE(bbox.ll.x == 10);
  REQUIRE(bbox.ll.y == 20);
  REQUIRE(bbox.ur.x == 50);
  REQUIRE(bbox.ur.y == 80);
}

TEST_CASE("PolyLine - Circumference", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  PolyLine polyline(points, true);

  // Perimeter of 100x100 square = 400
  double circumference = polyline.circumference();
  REQUIRE(circumference == 400.0);
}

TEST_CASE("PolyLine - Point containment (closed)", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  PolyLine polyline(points, true);

  REQUIRE(polyline.contains(IntPoint(50, 50)));    // Inside
  REQUIRE(!polyline.contains(IntPoint(150, 50)));  // Outside
  REQUIRE(!polyline.contains(IntPoint(-10, 50)));  // Outside
}

TEST_CASE("PolyLine - Distance calculation", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0)
  };

  PolyLine polyline(points);

  // Point directly above line segment
  IntPoint testPoint(50, 50);
  double dist = polyline.distance(testPoint);
  REQUIRE(dist == 50.0);
}

TEST_CASE("PolyLine - Nearest point", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0)
  };

  PolyLine polyline(points);

  IntPoint testPoint(50, 50);
  IntPoint nearest = polyline.nearestPoint(testPoint);
  REQUIRE(nearest == IntPoint(50, 0));
}

TEST_CASE("PolyLine - Line width", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0)
  };

  PolyLine polyline(points, false, 10);  // Width of 10

  REQUIRE(polyline.getLineWidth() == 10);

  // Bounding box should be expanded by half line width
  IntBox bbox = polyline.getBoundingBox();
  REQUIRE(bbox.ll.x == -5);
  REQUIRE(bbox.ll.y == -5);
  REQUIRE(bbox.ur.x == 105);
  REQUIRE(bbox.ur.y == 5);
}

TEST_CASE("PolyLine - Segment access", "[phase5][polyline]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100)
  };

  PolyLine polyline(points, false);

  REQUIRE(polyline.segmentCount() == 2);

  IntPoint p1, p2;
  polyline.getSegment(0, p1, p2);
  REQUIRE(p1 == IntPoint(0, 0));
  REQUIRE(p2 == IntPoint(100, 0));

  polyline.getSegment(1, p1, p2);
  REQUIRE(p1 == IntPoint(100, 0));
  REQUIRE(p2 == IntPoint(100, 100));
}

// ============================================================================
// ComplexPolygon Tests
// ============================================================================

TEST_CASE("ComplexPolygon - Simple polygon (no holes)", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ComplexPolygon polygon(outer);

  REQUIRE(polygon.holeCount() == 0);
  REQUIRE(!polygon.isEmpty());
  REQUIRE(polygon.dimension() == 2);
  REQUIRE(polygon.area() == 10000.0);  // 100x100 square
}

TEST_CASE("ComplexPolygon - Polygon with hole", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  std::vector<IntPoint> hole = {
    IntPoint(25, 25),
    IntPoint(75, 25),
    IntPoint(75, 75),
    IntPoint(25, 75)
  };

  ComplexPolygon polygon(outer);
  polygon.addHole(hole);

  REQUIRE(polygon.holeCount() == 1);

  // Area = outer area - hole area
  // = 10000 - 2500 = 7500
  REQUIRE(polygon.area() == 7500.0);
}

TEST_CASE("ComplexPolygon - Point containment with hole", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  std::vector<IntPoint> hole = {
    IntPoint(25, 25),
    IntPoint(75, 25),
    IntPoint(75, 75),
    IntPoint(25, 75)
  };

  ComplexPolygon polygon(outer);
  polygon.addHole(hole);

  REQUIRE(polygon.contains(IntPoint(10, 10)));   // Inside, not in hole
  REQUIRE(!polygon.contains(IntPoint(50, 50)));  // Inside hole
  REQUIRE(!polygon.contains(IntPoint(150, 50))); // Outside
}

TEST_CASE("ComplexPolygon - Bounding box", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(10, 20),
    IntPoint(90, 20),
    IntPoint(90, 80),
    IntPoint(10, 80)
  };

  ComplexPolygon polygon(outer);

  IntBox bbox = polygon.getBoundingBox();
  REQUIRE(bbox.ll.x == 10);
  REQUIRE(bbox.ll.y == 20);
  REQUIRE(bbox.ur.x == 90);
  REQUIRE(bbox.ur.y == 80);
}

TEST_CASE("ComplexPolygon - Circumference with holes", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  std::vector<IntPoint> hole = {
    IntPoint(25, 25),
    IntPoint(75, 25),
    IntPoint(75, 75),
    IntPoint(25, 75)
  };

  ComplexPolygon polygon(outer);
  polygon.addHole(hole);

  // Perimeter = outer perimeter + hole perimeter
  // = 400 + 200 = 600
  REQUIRE(polygon.circumference() == 600.0);
}

TEST_CASE("ComplexPolygon - Distance calculation", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ComplexPolygon polygon(outer);

  // Point inside has distance 0
  REQUIRE(polygon.distance(IntPoint(50, 50)) == 0.0);

  // Point outside has distance > 0
  double dist = polygon.distance(IntPoint(150, 50));
  REQUIRE(dist == 50.0);
}

// ============================================================================
// Board Integration Tests
// ============================================================================

TEST_CASE("BoardOutline - Shape integration", "[phase5][board][outline]") {
  BoardOutline outline(1, 2);

  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(1000, 0),
    IntPoint(1000, 800),
    IntPoint(0, 800)
  };

  PolyLine polyline(points, true);
  outline.addPolyLine(polyline);

  REQUIRE(outline.getShapes().size() == 1);
  REQUIRE(outline.contains(IntPoint(500, 400)));
  REQUIRE(!outline.contains(IntPoint(-10, 400)));

  IntBox bbox = outline.getBoundingBox();
  REQUIRE(bbox.ll.x == 0);
  REQUIRE(bbox.ll.y == 0);
  REQUIRE(bbox.ur.x == 1000);
  REQUIRE(bbox.ur.y == 800);
}

TEST_CASE("RuleArea - Shape integration", "[phase5][board][rulearea]") {
  RuleArea ruleArea(1, 0, "TestKeepout", false, false, false);

  std::vector<IntPoint> outer = {
    IntPoint(100, 100),
    IntPoint(200, 100),
    IntPoint(200, 200),
    IntPoint(100, 200)
  };

  ComplexPolygon polygon(outer);
  ruleArea.setPolygon(polygon);

  REQUIRE(ruleArea.getShape() != nullptr);
  REQUIRE(ruleArea.contains(IntPoint(150, 150)));
  REQUIRE(!ruleArea.contains(IntPoint(50, 50)));
  REQUIRE(!ruleArea.contains(IntPoint(250, 150)));

  IntBox bbox = ruleArea.getBoundingBox();
  REQUIRE(bbox.ll.x == 100);
  REQUIRE(bbox.ll.y == 100);
  REQUIRE(bbox.ur.x == 200);
  REQUIRE(bbox.ur.y == 200);
}

TEST_CASE("RuleArea - Restriction enforcement", "[phase5][board][rulearea]") {
  RuleArea ruleArea(1, 0, "NoTraces", false, true, true);

  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ruleArea.setPolygon(ComplexPolygon(outer));

  REQUIRE(!ruleArea.tracesAllowed());
  REQUIRE(ruleArea.viasAllowed());
  REQUIRE(ruleArea.copperPourAllowed());

  REQUIRE(ruleArea.isProhibited(RuleArea::RestrictionType::Traces));
  REQUIRE(!ruleArea.isProhibited(RuleArea::RestrictionType::Vias));
}

TEST_CASE("ConductionArea - Shape integration", "[phase5][board][conduction]") {
  std::vector<int> nets = {1};
  ConductionArea area(1, 0, nets, "CopperPour", true);

  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(500, 0),
    IntPoint(500, 500),
    IntPoint(0, 500)
  };

  std::vector<IntPoint> hole = {
    IntPoint(200, 200),
    IntPoint(300, 200),
    IntPoint(300, 300),
    IntPoint(200, 300)
  };

  ComplexPolygon polygon(outer);
  polygon.addHole(hole);
  area.setPolygon(polygon);

  REQUIRE(area.getShape() != nullptr);
  REQUIRE(area.contains(IntPoint(100, 100)));   // Inside copper
  REQUIRE(!area.contains(IntPoint(250, 250)));  // Inside hole
  REQUIRE(!area.contains(IntPoint(600, 100)));  // Outside

  IntBox bbox = area.getBoundingBox();
  REQUIRE(bbox.ll.x == 0);
  REQUIRE(bbox.ll.y == 0);
  REQUIRE(bbox.ur.x == 500);
  REQUIRE(bbox.ur.y == 500);
}

TEST_CASE("ComplexPolygon - Multiple holes", "[phase5][complexpolygon]") {
  std::vector<IntPoint> outer = {
    IntPoint(0, 0),
    IntPoint(200, 0),
    IntPoint(200, 200),
    IntPoint(0, 200)
  };

  std::vector<IntPoint> hole1 = {
    IntPoint(20, 20),
    IntPoint(80, 20),
    IntPoint(80, 80),
    IntPoint(20, 80)
  };

  std::vector<IntPoint> hole2 = {
    IntPoint(120, 120),
    IntPoint(180, 120),
    IntPoint(180, 180),
    IntPoint(120, 180)
  };

  ComplexPolygon polygon(outer);
  polygon.addHole(hole1);
  polygon.addHole(hole2);

  REQUIRE(polygon.holeCount() == 2);

  // Area = 40000 - 3600 - 3600 = 32800
  REQUIRE(polygon.area() == 32800.0);

  // Test containment
  REQUIRE(polygon.contains(IntPoint(10, 10)));    // Outside both holes
  REQUIRE(!polygon.contains(IntPoint(50, 50)));   // In hole 1
  REQUIRE(!polygon.contains(IntPoint(150, 150))); // In hole 2
  REQUIRE(polygon.contains(IntPoint(100, 100)));  // Between holes
}
