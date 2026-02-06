#include <catch2/catch_test_macros.hpp>
#include "autoroute/AutorouteControl.h"
#include "autoroute/DestinationDistance.h"
#include "autoroute/AutorouteEngine.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include "board/RoutingBoard.h"
#include "board/BasicBoard.h"
#include "board/LayerStructure.h"
#include "rules/ClearanceMatrix.h"
#include "geometry/Circle.h"
#include <thread>
#include <chrono>

using namespace freerouting;

// ============================================================================
// AutorouteControl Tests
// ============================================================================

TEST_CASE("AutorouteControl - Construction", "[autoroute][control]") {
  AutorouteControl ctrl(4);

  REQUIRE(ctrl.layerCount == 4);
  REQUIRE(ctrl.traceCosts.size() == 4);
  REQUIRE(ctrl.layerActive.size() == 4);
  REQUIRE(ctrl.traceHalfWidth.size() == 4);
  REQUIRE(ctrl.viasAllowed == true);
  REQUIRE(ctrl.minNormalViaCost > 0.0);
}

TEST_CASE("AutorouteControl - Default values", "[autoroute][control]") {
  AutorouteControl ctrl(2);

  // Check default costs
  REQUIRE(ctrl.traceCosts[0].horizontal == 1.0);
  REQUIRE(ctrl.traceCosts[0].vertical == 1.0);

  // Check default layer activation
  REQUIRE(ctrl.layerActive[0] == true);
  REQUIRE(ctrl.layerActive[1] == true);

  // Check default settings
  REQUIRE(ctrl.ripupAllowed == false);
  REQUIRE(ctrl.removeUnconnectedVias == true);
}

// ============================================================================
// DestinationDistance Tests
// ============================================================================

TEST_CASE("DestinationDistance - Construction", "[autoroute][destination]") {
  std::vector<AutorouteControl::ExpansionCostFactor> costs(2);
  costs[0] = AutorouteControl::ExpansionCostFactor(1.0, 1.0);
  costs[1] = AutorouteControl::ExpansionCostFactor(1.0, 1.0);

  std::vector<bool> layerActive = {true, true};

  DestinationDistance dist(costs, layerActive, 100.0, 50.0);

  // Should be initialized without crashes
}

TEST_CASE("DestinationDistance - Join and calculate", "[autoroute][destination]") {
  std::vector<AutorouteControl::ExpansionCostFactor> costs(2);
  costs[0] = AutorouteControl::ExpansionCostFactor(1.0, 1.0);
  costs[1] = AutorouteControl::ExpansionCostFactor(1.0, 1.0);

  std::vector<bool> layerActive = {true, true};

  DestinationDistance dist(costs, layerActive, 100.0, 50.0);

  // Add a destination box on layer 0
  IntBox destBox(100, 100, 200, 200);
  dist.join(destBox, 0);

  // Calculate distance from a point to destinations
  IntPoint point(0, 0);
  double distance = dist.calculate(point, 0);

  // Distance should be positive (Manhattan distance * cost)
  REQUIRE(distance > 0.0);

  // Point inside the box should have zero distance
  IntPoint insidePoint(150, 150);
  double insideDist = dist.calculate(insidePoint, 0);
  REQUIRE(insideDist == 0.0);
}

TEST_CASE("DestinationDistance - Multiple destinations", "[autoroute][destination]") {
  std::vector<AutorouteControl::ExpansionCostFactor> costs(3);
  for (int i = 0; i < 3; ++i) {
    costs[i] = AutorouteControl::ExpansionCostFactor(1.0, 1.0);
  }

  std::vector<bool> layerActive = {true, true, true};

  DestinationDistance dist(costs, layerActive, 100.0, 50.0);

  // Add destinations on different layers
  IntBox box1(100, 100, 200, 200);
  IntBox box2(300, 300, 400, 400);

  dist.join(box1, 0);  // Component side
  dist.join(box2, 2);  // Solder side

  // Calculate should return distance to nearest destination
  IntPoint point(0, 0);
  double distance = dist.calculate(point, 0);
  REQUIRE(distance > 0.0);
}

// ============================================================================
// Stoppable Tests
// ============================================================================

TEST_CASE("SimpleStoppable - Basic operations", "[autoroute][stoppable]") {
  SimpleStoppable stoppable;

  REQUIRE_FALSE(stoppable.isStopRequested());

  stoppable.requestStop();
  REQUIRE(stoppable.isStopRequested());

  stoppable.reset();
  REQUIRE_FALSE(stoppable.isStopRequested());
}

// ============================================================================
// TimeLimit Tests
// ============================================================================

TEST_CASE("TimeLimit - No limit", "[autoroute][timelimit]") {
  TimeLimit limit; // No limit

  REQUIRE_FALSE(limit.isExceeded());
  REQUIRE(limit.getRemainingMs() == -1); // Unlimited
}

TEST_CASE("TimeLimit - With limit", "[autoroute][timelimit]") {
  TimeLimit limit(100); // 100ms limit

  REQUIRE_FALSE(limit.isExceeded());

  // Wait for limit to exceed
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  REQUIRE(limit.isExceeded());
  REQUIRE(limit.getElapsedMs() >= 100);
}

TEST_CASE("TimeLimit - Reset", "[autoroute][timelimit]") {
  TimeLimit limit(50);

  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  REQUIRE(limit.isExceeded());

  limit.reset();
  REQUIRE_FALSE(limit.isExceeded());
}

// ============================================================================
// AutorouteEngine Tests
// ============================================================================

TEST_CASE("AutorouteEngine - Construction", "[autoroute][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));
  layers.addLayer(Layer("B.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(2, layers, classNames);

  RoutingBoard routingBoard(layers, clearanceMatrix);

  AutorouteEngine engine(&routingBoard, false);

  REQUIRE(engine.board == &routingBoard);
  REQUIRE(engine.maintainDatabase == false);
  REQUIRE(engine.getNetNo() == -1);
}

TEST_CASE("AutorouteEngine - Initialize connection", "[autoroute][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard routingBoard(layers, clearanceMatrix);
  AutorouteEngine engine(&routingBoard, false);

  SimpleStoppable stoppable;
  TimeLimit timeLimit(1000);

  engine.initConnection(1, &stoppable, &timeLimit);

  REQUIRE(engine.getNetNo() == 1);
  REQUIRE_FALSE(engine.isStopRequested());
}

TEST_CASE("AutorouteEngine - Stop request", "[autoroute][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard routingBoard(layers, clearanceMatrix);
  AutorouteEngine engine(&routingBoard, false);

  SimpleStoppable stoppable;
  TimeLimit timeLimit(10);

  engine.initConnection(1, &stoppable, &timeLimit);

  // Initially not stopped
  REQUIRE_FALSE(engine.isStopRequested());

  // Request stop via stoppable
  stoppable.requestStop();
  REQUIRE(engine.isStopRequested());
}

TEST_CASE("AutorouteEngine - Expansion rooms", "[autoroute][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard routingBoard(layers, clearanceMatrix);
  AutorouteEngine engine(&routingBoard, false);

  // Add an incomplete expansion room
  Circle outerShape(IntPoint(0, 0), 100);
  Circle innerShape(IntPoint(0, 0), 50);

  auto* room = engine.addIncompleteExpansionRoom(&outerShape, 0, &innerShape);

  REQUIRE(room != nullptr);
  REQUIRE(engine.getFirstIncompleteExpansionRoom() == room);

  // Clear
  engine.clear();
  REQUIRE(engine.getFirstIncompleteExpansionRoom() == nullptr);
}
