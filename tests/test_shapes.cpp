#include <catch2/catch_test_macros.hpp>
#include "geometry/Shape.h"
#include "geometry/Circle.h"
#include "geometry/ConvexPolygon.h"
#include "geometry/SpatialIndex.h"
#include "geometry/ShapeTree.h"
#include "geometry/CollisionDetector.h"
#include "board/BasicBoard.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "rules/ClearanceMatrix.h"

using namespace freerouting;

// ============================================================================
// Circle Tests
// ============================================================================

TEST_CASE("Circle - Basic properties", "[shapes][circle]") {
  Circle circle(IntPoint(100, 200), 50);

  REQUIRE(circle.getCenter() == IntPoint(100, 200));
  REQUIRE(circle.getRadius() == 50);
  REQUIRE(!circle.isEmpty());
  REQUIRE(circle.dimension() == 2);
}

TEST_CASE("Circle - Area and circumference", "[shapes][circle]") {
  Circle circle(IntPoint(0, 0), 100);

  double area = circle.area();
  double circumference = circle.circumference();

  // Area = pi * r^2 = pi * 10000 ≈ 31415.9
  REQUIRE(area > 31400.0);
  REQUIRE(area < 31450.0);

  // Circumference = 2 * pi * r = 2 * pi * 100 ≈ 628.3
  REQUIRE(circumference > 628.0);
  REQUIRE(circumference < 629.0);
}

TEST_CASE("Circle - Bounding box", "[shapes][circle]") {
  Circle circle(IntPoint(100, 200), 50);
  IntBox bbox = circle.getBoundingBox();

  REQUIRE(bbox.ll == IntPoint(50, 150));
  REQUIRE(bbox.ur == IntPoint(150, 250));
}

TEST_CASE("Circle - Point containment", "[shapes][circle]") {
  Circle circle(IntPoint(0, 0), 100);

  // Point at center
  REQUIRE(circle.contains(IntPoint(0, 0)));
  REQUIRE(circle.containsInside(IntPoint(0, 0)));

  // Point on boundary
  REQUIRE(circle.contains(IntPoint(100, 0)));
  REQUIRE(!circle.containsInside(IntPoint(100, 0)));

  // Point inside
  REQUIRE(circle.contains(IntPoint(50, 50)));
  REQUIRE(circle.containsInside(IntPoint(50, 50)));

  // Point outside
  REQUIRE(!circle.contains(IntPoint(200, 200)));
  REQUIRE(!circle.containsInside(IntPoint(200, 200)));
}

TEST_CASE("Circle - Distance calculations", "[shapes][circle]") {
  Circle circle(IntPoint(0, 0), 100);

  // Point inside
  REQUIRE(circle.distance(IntPoint(0, 0)) == 0.0);

  // Point on boundary
  REQUIRE(circle.distance(IntPoint(100, 0)) == 0.0);

  // Point outside along x-axis
  double dist = circle.distance(IntPoint(200, 0));
  REQUIRE(dist > 99.9);
  REQUIRE(dist < 100.1);
}

TEST_CASE("Circle - Nearest point", "[shapes][circle]") {
  Circle circle(IntPoint(0, 0), 100);

  // Point outside along x-axis
  IntPoint nearest = circle.nearestPoint(IntPoint(200, 0));
  REQUIRE(nearest == IntPoint(100, 0));

  // Point outside along y-axis
  nearest = circle.nearestPoint(IntPoint(0, -200));
  REQUIRE(nearest == IntPoint(0, -100));
}

TEST_CASE("Circle - Circle intersection", "[shapes][circle]") {
  Circle c1(IntPoint(0, 0), 100);
  Circle c2(IntPoint(150, 0), 100);
  Circle c3(IntPoint(300, 0), 100);

  // Overlapping circles
  REQUIRE(c1.intersects(c2));

  // Touching circles
  REQUIRE(c1.intersects(c2));

  // Non-intersecting circles
  REQUIRE(!c1.intersects(c3));
}

TEST_CASE("Circle - Box intersection", "[shapes][circle]") {
  Circle circle(IntPoint(100, 100), 50);

  // Box containing circle
  IntBox box1(0, 0, 200, 200);
  REQUIRE(circle.intersects(box1));

  // Box intersecting circle
  IntBox box2(120, 80, 200, 120);
  REQUIRE(circle.intersects(box2));

  // Box not intersecting
  IntBox box3(200, 200, 300, 300);
  REQUIRE(!circle.intersects(box3));
}

TEST_CASE("Circle - Offset", "[shapes][circle]") {
  Circle circle(IntPoint(100, 100), 50);

  Circle larger = circle.offset(10);
  REQUIRE(larger.getRadius() == 60);
  REQUIRE(larger.getCenter() == IntPoint(100, 100));

  Circle smaller = circle.offset(-10);
  REQUIRE(smaller.getRadius() == 40);
}

// ============================================================================
// ConvexPolygon Tests
// ============================================================================

TEST_CASE("ConvexPolygon - Triangle", "[shapes][polygon]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(50, 100)
  };

  ConvexPolygon triangle(points);

  REQUIRE(triangle.vertexCount() == 3);
  REQUIRE(!triangle.isEmpty());
  REQUIRE(triangle.dimension() == 2);
}

TEST_CASE("ConvexPolygon - Removes duplicates", "[shapes][polygon]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(0, 0),  // Duplicate
    IntPoint(100, 0),
    IntPoint(50, 100)
  };

  ConvexPolygon polygon(points);
  REQUIRE(polygon.vertexCount() == 3);
}

TEST_CASE("ConvexPolygon - Removes collinear points", "[shapes][polygon]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(50, 0),   // Collinear with 0,0 and 100,0
    IntPoint(100, 0),
    IntPoint(100, 100)
  };

  ConvexPolygon polygon(points);
  // Should remove (50, 0)
  REQUIRE(polygon.vertexCount() == 3);
}

TEST_CASE("ConvexPolygon - Area calculation", "[shapes][polygon]") {
  // Square 100x100
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ConvexPolygon square(points);
  REQUIRE(square.area() == 10000.0);
}

TEST_CASE("ConvexPolygon - Perimeter calculation", "[shapes][polygon]") {
  // Square 100x100
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ConvexPolygon square(points);
  REQUIRE(square.circumference() == 400.0);
}

TEST_CASE("ConvexPolygon - Bounding box", "[shapes][polygon]") {
  std::vector<IntPoint> points = {
    IntPoint(10, 20),
    IntPoint(100, 30),
    IntPoint(90, 150),
    IntPoint(20, 140)
  };

  ConvexPolygon polygon(points);
  IntBox bbox = polygon.getBoundingBox();

  REQUIRE(bbox.ll == IntPoint(10, 20));
  REQUIRE(bbox.ur == IntPoint(100, 150));
}

TEST_CASE("ConvexPolygon - Point containment", "[shapes][polygon]") {
  // Square 100x100
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ConvexPolygon square(points);

  // Point inside
  REQUIRE(square.contains(IntPoint(50, 50)));

  // Point on edge
  REQUIRE(square.contains(IntPoint(50, 0)));

  // Point outside
  REQUIRE(!square.contains(IntPoint(150, 150)));
  REQUIRE(!square.contains(IntPoint(-10, 50)));
}

TEST_CASE("ConvexPolygon - Box intersection", "[shapes][polygon]") {
  std::vector<IntPoint> points = {
    IntPoint(0, 0),
    IntPoint(100, 0),
    IntPoint(100, 100),
    IntPoint(0, 100)
  };

  ConvexPolygon square(points);

  // Box overlapping
  IntBox box1(50, 50, 150, 150);
  REQUIRE(square.intersects(box1));

  // Box contained
  IntBox box2(25, 25, 75, 75);
  REQUIRE(square.intersects(box2));

  // Box not intersecting
  IntBox box3(200, 200, 300, 300);
  REQUIRE(!square.intersects(box3));
}

// ============================================================================
// SpatialIndex Tests
// ============================================================================

TEST_CASE("SpatialIndex - Insert and query", "[shapes][spatial]") {
  SpatialIndex<int> index(1000);  // Larger cell size to avoid boundary issues

  int item1 = 1;
  int item2 = 2;
  int item3 = 3;

  IntBox box1(0, 0, 50, 50);
  IntBox box2(1100, 1100, 1150, 1150);  // Clearly in different cell
  IntBox box3(2200, 2200, 2250, 2250);  // Clearly in different cell

  index.insert(&item1, box1);
  index.insert(&item2, box2);
  index.insert(&item3, box3);

  // Query region containing only item1
  auto result1 = index.query(IntBox(0, 0, 100, 100));
  REQUIRE(result1.size() == 1);
  REQUIRE(*result1[0] == 1);

  // Query region containing item1 and item2 (but not item3)
  auto result2 = index.query(IntBox(0, 0, 1999, 1999));
  REQUIRE(result2.size() == 2);

  // Query region containing all items
  auto result3 = index.query(IntBox(0, 0, 3000, 3000));
  REQUIRE(result3.size() == 3);
}

TEST_CASE("SpatialIndex - Query near point", "[shapes][spatial]") {
  SpatialIndex<int> index(100);

  int item1 = 1;
  int item2 = 2;

  IntBox box1(0, 0, 10, 10);
  IntBox box2(1000, 1000, 1010, 1010);

  index.insert(&item1, box1);
  index.insert(&item2, box2);

  // Query near item1
  auto result1 = index.queryNear(IntPoint(5, 5), 50);
  REQUIRE(result1.size() == 1);
  REQUIRE(*result1[0] == 1);

  // Query near item2
  auto result2 = index.queryNear(IntPoint(1005, 1005), 50);
  REQUIRE(result2.size() == 1);
  REQUIRE(*result2[0] == 2);
}

TEST_CASE("SpatialIndex - Remove item", "[shapes][spatial]") {
  SpatialIndex<int> index(100);

  int item1 = 1;
  int item2 = 2;

  IntBox box1(0, 0, 50, 50);
  IntBox box2(100, 100, 150, 150);

  index.insert(&item1, box1);
  index.insert(&item2, box2);

  // Remove item1
  index.remove(&item1, box1);

  auto result = index.query(IntBox(0, 0, 200, 200));
  REQUIRE(result.size() == 1);
  REQUIRE(*result[0] == 2);
}

TEST_CASE("SpatialIndex - Clear", "[shapes][spatial]") {
  SpatialIndex<int> index(100);

  int item1 = 1;
  int item2 = 2;

  index.insert(&item1, IntBox(0, 0, 50, 50));
  index.insert(&item2, IntBox(100, 100, 150, 150));

  index.clear();

  auto result = index.query(IntBox(0, 0, 300, 300));
  REQUIRE(result.empty());
}

// ============================================================================
// ShapeTree Tests
// ============================================================================

TEST_CASE("ShapeTree - Insert and query items", "[shapes][shapetree]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  BasicBoard board(layers, clearanceMatrix);

  ShapeTree tree(10000);  // Larger cell size

  // Create some traces far apart
  auto trace1 = std::make_unique<Trace>(
    IntPoint(0, 0), IntPoint(100, 0), 0, 5,
    std::vector<int>{1}, 0, 1, FixedState::NotFixed, &board
  );

  auto trace2 = std::make_unique<Trace>(
    IntPoint(15000, 15000), IntPoint(15100, 15000), 0, 5,
    std::vector<int>{2}, 0, 2, FixedState::NotFixed, &board
  );

  tree.insert(trace1.get());
  tree.insert(trace2.get());

  // Query region containing only trace1
  auto result1 = tree.queryRegion(IntBox(-10, -10, 200, 100));
  REQUIRE(result1.size() == 1);
  REQUIRE(result1[0]->getId() == 1);

  // Query region containing both
  auto result2 = tree.queryRegion(IntBox(0, 0, 20000, 20000));
  REQUIRE(result2.size() == 2);
}

TEST_CASE("ShapeTree - Find obstacles", "[shapes][shapetree]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  BasicBoard board(layers, clearanceMatrix);

  ShapeTree tree(1000);

  // Create traces on different nets
  auto trace1 = std::make_unique<Trace>(
    IntPoint(0, 0), IntPoint(100, 0), 0, 5,
    std::vector<int>{1}, 0, 1, FixedState::NotFixed, &board
  );

  auto trace2 = std::make_unique<Trace>(
    IntPoint(50, -10), IntPoint(50, 10), 0, 5,
    std::vector<int>{2}, 0, 2, FixedState::NotFixed, &board
  );

  tree.insert(trace1.get());
  tree.insert(trace2.get());

  // Find obstacles for trace1 (should find trace2)
  auto obstacles = tree.findObstacles(*trace1);
  REQUIRE(obstacles.size() == 1);
  REQUIRE(obstacles[0]->getId() == 2);
}

// ============================================================================
// CollisionDetector Tests
// ============================================================================

TEST_CASE("CollisionDetector - Circle-circle intersection", "[shapes][collision]") {
  Circle c1(IntPoint(0, 0), 50);
  Circle c2(IntPoint(75, 0), 50);
  Circle c3(IntPoint(200, 0), 50);

  REQUIRE(CollisionDetector::circlesIntersect(c1, c2));
  REQUIRE(!CollisionDetector::circlesIntersect(c1, c3));
}

TEST_CASE("CollisionDetector - Circle-box intersection", "[shapes][collision]") {
  Circle circle(IntPoint(100, 100), 50);
  IntBox box1(50, 50, 120, 120);
  IntBox box2(200, 200, 300, 300);

  REQUIRE(CollisionDetector::circleBoxIntersect(circle, box1));
  REQUIRE(!CollisionDetector::circleBoxIntersect(circle, box2));
}

TEST_CASE("CollisionDetector - Box-box intersection", "[shapes][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(50, 50, 150, 150);
  IntBox box3(200, 200, 300, 300);

  REQUIRE(CollisionDetector::boxesIntersect(box1, box2));
  REQUIRE(!CollisionDetector::boxesIntersect(box1, box3));
}

TEST_CASE("CollisionDetector - Box distance", "[shapes][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(200, 0, 300, 100);
  IntBox box3(50, 50, 150, 150);

  // Adjacent boxes (horizontal gap)
  double dist1 = CollisionDetector::boxDistance(box1, box2);
  REQUIRE(dist1 == 100.0);

  // Overlapping boxes
  double dist2 = CollisionDetector::boxDistance(box1, box3);
  REQUIRE(dist2 == 0.0);
}

TEST_CASE("CollisionDetector - Circle distance", "[shapes][collision]") {
  Circle c1(IntPoint(0, 0), 50);
  Circle c2(IntPoint(200, 0), 50);
  Circle c3(IntPoint(75, 0), 50);

  // Non-intersecting circles
  double dist1 = CollisionDetector::circleDistance(c1, c2);
  REQUIRE(dist1 > 99.9);
  REQUIRE(dist1 < 100.1);

  // Intersecting circles
  double dist2 = CollisionDetector::circleDistance(c1, c3);
  REQUIRE(dist2 == 0.0);
}

TEST_CASE("CollisionDetector - Segment-circle intersection", "[shapes][collision]") {
  Circle circle(IntPoint(100, 100), 50);

  // Segment passing through circle
  REQUIRE(CollisionDetector::segmentCircleIntersect(
    IntPoint(50, 100), IntPoint(150, 100), circle));

  // Segment missing circle
  REQUIRE(!CollisionDetector::segmentCircleIntersect(
    IntPoint(50, 200), IntPoint(150, 200), circle));

  // Segment endpoint in circle
  REQUIRE(CollisionDetector::segmentCircleIntersect(
    IntPoint(100, 100), IntPoint(200, 200), circle));
}

TEST_CASE("CollisionDetector - Clearance checks", "[shapes][collision]") {
  IntBox box1(0, 0, 100, 100);
  IntBox box2(110, 0, 210, 100);

  // 10 unit gap, so 5 unit clearance fails, 15 succeeds
  REQUIRE(!CollisionDetector::boxesWithinClearance(box1, box2, 5));
  REQUIRE(CollisionDetector::boxesWithinClearance(box1, box2, 15));

  Circle c1(IntPoint(0, 0), 50);
  Circle c2(IntPoint(200, 0), 50);

  // 100 unit gap between surfaces
  REQUIRE(!CollisionDetector::circlesWithinClearance(c1, c2, 50));
  REQUIRE(CollisionDetector::circlesWithinClearance(c1, c2, 150));
}
