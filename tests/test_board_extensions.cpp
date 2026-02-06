#include <catch2/catch_test_macros.hpp>
#include "board/Component.h"
#include "board/BoardOutline.h"
#include "board/ConductionArea.h"

using namespace freerouting;

// ============================================================================
// Component Tests
// ============================================================================

TEST_CASE("Component - Construction", "[board][component]") {
  IntPoint location(1000, 2000);
  Component comp("U1", location, 90.0, true, 1, false);

  REQUIRE(comp.getName() == "U1");
  REQUIRE(comp.getNo() == 1);
  REQUIRE(comp.getLocation() == location);
  REQUIRE(comp.getRotation() == 90.0);
  REQUIRE(comp.isOnFront());
  REQUIRE_FALSE(comp.isPositionFixed());
  REQUIRE(comp.isPlaced());
}

TEST_CASE("Component - Rotation normalization", "[board][component]") {
  // Test rotation wrapping to [0, 360)
  IntPoint location(0, 0);

  // Positive overflow
  Component comp1("U1", location, 450.0, true, 1, false);
  REQUIRE(comp1.getRotation() == 90.0);

  // Negative wrapping
  Component comp2("U2", location, -90.0, true, 2, false);
  REQUIRE(comp2.getRotation() == 270.0);

  // Multiple wraps
  Component comp3("U3", location, 720.0, true, 3, false);
  REQUIRE(comp3.getRotation() == 0.0);
}

TEST_CASE("Component - Front vs back placement", "[board][component]") {
  IntPoint location(0, 0);

  Component frontComp("U1", location, 0.0, true, 1, false);
  REQUIRE(frontComp.isOnFront());

  Component backComp("U2", location, 0.0, false, 2, false);
  REQUIRE_FALSE(backComp.isOnFront());
}

TEST_CASE("Component - Fixed position", "[board][component]") {
  IntPoint location(0, 0);

  Component movable("U1", location, 0.0, true, 1, false);
  REQUIRE_FALSE(movable.isPositionFixed());

  Component fixed("U2", location, 0.0, true, 2, true);
  REQUIRE(fixed.isPositionFixed());
}

// ============================================================================
// BoardOutline Tests
// ============================================================================

TEST_CASE("BoardOutline - Construction", "[board][outline]") {
  BoardOutline outline(1, 4);  // 4 layers

  REQUIRE(outline.getId() == 1);
  REQUIRE(outline.hasKeepoutOutside());
}

TEST_CASE("BoardOutline - Layer span", "[board][outline]") {
  BoardOutline outline(1, 4);  // 4 layers

  // Outlines span all layers
  REQUIRE(outline.firstLayer() == 0);
  REQUIRE(outline.lastLayer() == 3);
  REQUIRE(outline.layerCount() == 4);
}

TEST_CASE("BoardOutline - Obstacle behavior", "[board][outline]") {
  BoardOutline outline1(1, 2);
  BoardOutline outline2(2, 2);

  // Board outline is obstacle to everything
  REQUIRE(outline1.isObstacle(outline2));
}

// ============================================================================
// ConductionArea Tests
// ============================================================================

TEST_CASE("ConductionArea - Construction", "[board][area]") {
  std::vector<int> nets = {1};
  ConductionArea area(1, 0, nets, "GND_POUR", true);

  REQUIRE(area.getId() == 1);
  REQUIRE(area.firstLayer() == 0);
  REQUIRE(area.lastLayer() == 0);
  REQUIRE(area.getName() == "GND_POUR");
  REQUIRE(area.getIsObstacle());
}

TEST_CASE("ConductionArea - Obstacle behavior", "[board][area]") {
  std::vector<int> nets1 = {1};
  std::vector<int> nets2 = {2};

  ConductionArea area1(1, 0, nets1, "Area1", true);
  ConductionArea area2(2, 0, nets2, "Area2", false);

  // area1 is obstacle, area2 is not
  REQUIRE(area1.getIsObstacle());
  REQUIRE_FALSE(area2.getIsObstacle());

  // area1 should be obstacle to items on different net
  REQUIRE(area1.isObstacle(area2));

  // area2 should not be obstacle even to different nets
  REQUIRE_FALSE(area2.isObstacle(area1));
}

TEST_CASE("ConductionArea - Same net interaction", "[board][area]") {
  std::vector<int> nets = {1};

  ConductionArea area1(1, 0, nets, "Area1", true);
  ConductionArea area2(2, 0, nets, "Area2", true);

  // Areas on same net should not be obstacles to each other
  REQUIRE_FALSE(area1.isObstacle(area2));
  REQUIRE_FALSE(area2.isObstacle(area1));
}

TEST_CASE("ConductionArea - Set obstacle flag", "[board][area]") {
  std::vector<int> nets = {1};
  ConductionArea area(1, 0, nets, "Area", true);

  REQUIRE(area.getIsObstacle());

  area.setIsObstacle(false);
  REQUIRE_FALSE(area.getIsObstacle());

  area.setIsObstacle(true);
  REQUIRE(area.getIsObstacle());
}

TEST_CASE("ConductionArea - Layer properties", "[board][area]") {
  std::vector<int> nets = {1};
  ConductionArea area(1, 3, nets, "Area", true);

  // Conduction areas are on a single layer
  REQUIRE(area.firstLayer() == 3);
  REQUIRE(area.lastLayer() == 3);
  REQUIRE(area.layerCount() == 1);
}
