#include <catch2/catch_test_macros.hpp>
#include "autoroute/MazeSearchElement.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/FreeSpaceExpansionRoom.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "geometry/IntBox.h"
#include "geometry/Circle.h"
#include "board/Trace.h"
#include "board/BasicBoard.h"
#include "board/LayerStructure.h"
#include "rules/ClearanceMatrix.h"

using namespace freerouting;

// ============================================================================
// MazeSearchElement Tests
// ============================================================================

TEST_CASE("MazeSearchElement - Default construction", "[expansion][maze]") {
  MazeSearchElement element;

  REQUIRE_FALSE(element.isOccupied);
  REQUIRE(element.backtrackDoor == nullptr);
  REQUIRE(element.sectionNoOfBacktrackDoor == 0);
  REQUIRE_FALSE(element.roomRipped);
  REQUIRE(element.adjustment == MazeSearchElement::Adjustment::None);
}

TEST_CASE("MazeSearchElement - Reset", "[expansion][maze]") {
  MazeSearchElement element;

  // Modify element
  element.isOccupied = true;
  element.backtrackDoor = reinterpret_cast<ExpandableObject*>(0x1234);
  element.sectionNoOfBacktrackDoor = 5;
  element.roomRipped = true;
  element.adjustment = MazeSearchElement::Adjustment::Right;

  // Reset
  element.reset();

  // Verify reset
  REQUIRE_FALSE(element.isOccupied);
  REQUIRE(element.backtrackDoor == nullptr);
  REQUIRE(element.sectionNoOfBacktrackDoor == 0);
  REQUIRE_FALSE(element.roomRipped);
  REQUIRE(element.adjustment == MazeSearchElement::Adjustment::None);
}

// ============================================================================
// FreeSpaceExpansionRoom Tests
// ============================================================================

TEST_CASE("FreeSpaceExpansionRoom - Basic operations", "[expansion][room]") {
  Circle shape(IntPoint(50, 50), 50);
  FreeSpaceExpansionRoom room(&shape, 0);

  REQUIRE(room.getLayer() == 0);
  REQUIRE(room.getShape() == &shape);
  REQUIRE(room.getDoors().empty());
}

TEST_CASE("FreeSpaceExpansionRoom - Door management", "[expansion][room]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);
  FreeSpaceExpansionRoom room1(&shape1, 0);
  FreeSpaceExpansionRoom room2(&shape2, 0);

  ExpansionDoor door(&room1, &room2, 1);

  room1.addDoor(&door);
  REQUIRE(room1.getDoors().size() == 1);

  REQUIRE(room1.doorExists(&room2));
  REQUIRE_FALSE(room1.doorExists(&room1));
}

TEST_CASE("FreeSpaceExpansionRoom - Clear doors", "[expansion][room]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);
  FreeSpaceExpansionRoom room1(&shape1, 0);
  FreeSpaceExpansionRoom room2(&shape2, 0);

  ExpansionDoor door(&room1, &room2, 1);
  room1.addDoor(&door);

  REQUIRE(room1.getDoors().size() == 1);

  room1.clearDoors();
  REQUIRE(room1.getDoors().empty());
}

// ============================================================================
// CompleteFreeSpaceExpansionRoom Tests
// ============================================================================

TEST_CASE("CompleteFreeSpaceExpansionRoom - Construction", "[expansion][complete]") {
  Circle shape(IntPoint(50, 50), 50);
  CompleteFreeSpaceExpansionRoom room(&shape, 0, 1);

  REQUIRE(room.getIdNo() == 1);
  REQUIRE(room.getLayer() == 0);
  REQUIRE_FALSE(room.isNetDependent());
  REQUIRE(room.getTargetDoors().empty());
}

TEST_CASE("CompleteFreeSpaceExpansionRoom - Net dependency", "[expansion][complete]") {
  Circle shape(IntPoint(50, 50), 50);
  CompleteFreeSpaceExpansionRoom room(&shape, 0, 1);

  REQUIRE_FALSE(room.isNetDependent());

  room.setNetDependent();
  REQUIRE(room.isNetDependent());
}

TEST_CASE("CompleteFreeSpaceExpansionRoom - Comparison", "[expansion][complete]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);

  CompleteFreeSpaceExpansionRoom room1(&shape1, 0, 1);
  CompleteFreeSpaceExpansionRoom room2(&shape2, 0, 2);

  REQUIRE(room1 < room2);
  REQUIRE_FALSE(room2 < room1);
}

// ============================================================================
// IncompleteFreeSpaceExpansionRoom Tests
// ============================================================================

TEST_CASE("IncompleteFreeSpaceExpansionRoom - Construction", "[expansion][incomplete]") {
  Circle outerShape(IntPoint(50, 50), 50);
  Circle innerShape(IntPoint(50, 50), 25);

  IncompleteFreeSpaceExpansionRoom room(&outerShape, 0, &innerShape);

  REQUIRE(room.getLayer() == 0);
  REQUIRE(room.getShape() == &outerShape);
  REQUIRE(room.getContainedShape() == &innerShape);
}

TEST_CASE("IncompleteFreeSpaceExpansionRoom - Set contained shape", "[expansion][incomplete]") {
  Circle outerShape(IntPoint(50, 50), 50);
  Circle innerShape1(IntPoint(50, 50), 25);
  Circle innerShape2(IntPoint(50, 50), 20);

  IncompleteFreeSpaceExpansionRoom room(&outerShape, 0, &innerShape1);

  REQUIRE(room.getContainedShape() == &innerShape1);

  room.setContainedShape(&innerShape2);
  REQUIRE(room.getContainedShape() == &innerShape2);
}

// ============================================================================
// ExpansionDoor Tests
// ============================================================================

TEST_CASE("ExpansionDoor - Construction", "[expansion][door]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);
  FreeSpaceExpansionRoom room1(&shape1, 0);
  FreeSpaceExpansionRoom room2(&shape2, 0);

  ExpansionDoor door(&room1, &room2, 1);

  REQUIRE(door.firstRoom == &room1);
  REQUIRE(door.secondRoom == &room2);
  REQUIRE(door.getDimension() == 1);
}

TEST_CASE("ExpansionDoor - Other room", "[expansion][door]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);
  FreeSpaceExpansionRoom room1(&shape1, 0);
  FreeSpaceExpansionRoom room2(&shape2, 0);

  ExpansionDoor door(&room1, &room2, 1);

  REQUIRE(door.otherRoom(&room1) == &room2);
  REQUIRE(door.otherRoom(&room2) == &room1);

  Circle shape3(IntPoint(150, 150), 50);
  FreeSpaceExpansionRoom room3(&shape3, 0);
  REQUIRE(door.otherRoom(&room3) == nullptr);
}

TEST_CASE("ExpansionDoor - Section allocation", "[expansion][door]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);
  FreeSpaceExpansionRoom room1(&shape1, 0);
  FreeSpaceExpansionRoom room2(&shape2, 0);

  ExpansionDoor door(&room1, &room2, 1);

  REQUIRE(door.mazeSearchElementCount() == 0);

  door.allocateSections(5);
  REQUIRE(door.mazeSearchElementCount() == 5);

  // Re-allocating same size shouldn't change anything
  door.allocateSections(5);
  REQUIRE(door.mazeSearchElementCount() == 5);
}

TEST_CASE("ExpansionDoor - Reset", "[expansion][door]") {
  Circle shape1(IntPoint(50, 50), 50);
  Circle shape2(IntPoint(100, 100), 50);
  FreeSpaceExpansionRoom room1(&shape1, 0);
  FreeSpaceExpansionRoom room2(&shape2, 0);

  ExpansionDoor door(&room1, &room2, 1);
  door.allocateSections(3);

  // Modify sections
  door.getMazeSearchElement(0).isOccupied = true;
  door.getMazeSearchElement(1).roomRipped = true;

  // Reset
  door.reset();

  // Verify reset
  for (int i = 0; i < door.mazeSearchElementCount(); ++i) {
    REQUIRE_FALSE(door.getMazeSearchElement(i).isOccupied);
    REQUIRE_FALSE(door.getMazeSearchElement(i).roomRipped);
  }
}

// ============================================================================
// TargetItemExpansionDoor Tests
// ============================================================================

TEST_CASE("TargetItemExpansionDoor - Construction", "[expansion][target]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  Circle roomShape(IntPoint(50, 50), 50);
  CompleteFreeSpaceExpansionRoom room(&roomShape, 0, 1);

  auto trace = std::make_unique<Trace>(
    IntPoint(0, 0), IntPoint(100, 0), 0, 5,
    std::vector<int>{1}, 0, 1, FixedState::NotFixed, &board
  );

  Circle doorShape(IntPoint(50, 50), 10);
  TargetItemExpansionDoor door(trace.get(), 0, &room, &doorShape);

  REQUIRE(door.item == trace.get());
  REQUIRE(door.treeEntryNo == 0);
  REQUIRE(door.room == &room);
  REQUIRE(door.getDimension() == 2);
  REQUIRE(door.mazeSearchElementCount() == 1);
}

TEST_CASE("TargetItemExpansionDoor - Reset", "[expansion][target]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  Circle roomShape(IntPoint(50, 50), 50);
  CompleteFreeSpaceExpansionRoom room(&roomShape, 0, 1);

  auto trace = std::make_unique<Trace>(
    IntPoint(0, 0), IntPoint(100, 0), 0, 5,
    std::vector<int>{1}, 0, 1, FixedState::NotFixed, &board
  );

  Circle doorShape(IntPoint(50, 50), 10);
  TargetItemExpansionDoor door(trace.get(), 0, &room, &doorShape);

  // Modify element
  door.getMazeSearchElement(0).isOccupied = true;

  // Reset
  door.reset();

  // Verify reset
  REQUIRE_FALSE(door.getMazeSearchElement(0).isOccupied);
}
