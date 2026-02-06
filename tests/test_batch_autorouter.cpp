#include <catch2/catch_test_macros.hpp>
#include "autoroute/BatchAutorouter.h"
#include "autoroute/AutorouteAttemptState.h"
#include "autoroute/AutorouteAttemptResult.h"
#include "autoroute/TaskState.h"
#include "autoroute/ItemRouteResult.h"
#include "autoroute/Connection.h"
#include "autoroute/ItemAutorouteInfo.h"
#include "board/RoutingBoard.h"
#include "board/LayerStructure.h"
#include "rules/ClearanceMatrix.h"
#include "datastructures/Stoppable.h"

using namespace freerouting;

// ============================================================================
// AutorouteAttemptState Tests
// ============================================================================

TEST_CASE("AutorouteAttemptState - Enum values", "[autoroute][batch][state]") {
  // Just verify enum exists and can be assigned
  AutorouteAttemptState state = AutorouteAttemptState::Routed;
  REQUIRE(state == AutorouteAttemptState::Routed);

  state = AutorouteAttemptState::Failed;
  REQUIRE(state == AutorouteAttemptState::Failed);

  state = AutorouteAttemptState::AlreadyConnected;
  REQUIRE(state == AutorouteAttemptState::AlreadyConnected);
}

// ============================================================================
// AutorouteAttemptResult Tests
// ============================================================================

TEST_CASE("AutorouteAttemptResult - Construction with state only", "[autoroute][batch][result]") {
  AutorouteAttemptResult result(AutorouteAttemptState::Routed);

  REQUIRE(result.state == AutorouteAttemptState::Routed);
  REQUIRE(result.details.empty());
}

TEST_CASE("AutorouteAttemptResult - Construction with details", "[autoroute][batch][result]") {
  AutorouteAttemptResult result(AutorouteAttemptState::Failed,
                                 "No valid path found");

  REQUIRE(result.state == AutorouteAttemptState::Failed);
  REQUIRE(result.details == "No valid path found");
}

// ============================================================================
// TaskState Tests
// ============================================================================

TEST_CASE("TaskState - Enum values", "[autoroute][batch][task]") {
  TaskState state = TaskState::Started;
  REQUIRE(state == TaskState::Started);

  state = TaskState::Running;
  REQUIRE(state == TaskState::Running);

  state = TaskState::Finished;
  REQUIRE(state == TaskState::Finished);

  state = TaskState::Cancelled;
  REQUIRE(state == TaskState::Cancelled);
}

// ============================================================================
// ItemRouteResult Tests
// ============================================================================

TEST_CASE("ItemRouteResult - Failed/skipped constructor", "[autoroute][batch][itemresult]") {
  ItemRouteResult result(42);

  REQUIRE(result.getItemId() == 42);
  REQUIRE_FALSE(result.isImproved());
  REQUIRE(result.getViaCount() == 0);
  REQUIRE(result.getTraceLength() == 0.0);
}

TEST_CASE("ItemRouteResult - Improved routing", "[autoroute][batch][itemresult]") {
  // Route that reduced vias and trace length
  ItemRouteResult result(1, 10, 5, 1000.0, 800.0, 0, 0);

  REQUIRE(result.isImproved());
  REQUIRE(result.getViaCountReduced() == 5);
  REQUIRE(result.getLengthReduced() > 0.0);
  REQUIRE(result.getImprovementPercentage() > 0.0f);
}

TEST_CASE("ItemRouteResult - Worsened routing", "[autoroute][batch][itemresult]") {
  // Route that added vias
  ItemRouteResult result(1, 5, 10, 800.0, 1000.0, 0, 0);

  REQUIRE_FALSE(result.isImproved());
  REQUIRE(result.getViaCountReduced() == -5);
  REQUIRE(result.getLengthReduced() < 0.0);
}

TEST_CASE("ItemRouteResult - Incomplete connection improvement", "[autoroute][batch][itemresult]") {
  // Routing that connected previously unconnected items
  ItemRouteResult result(1, 5, 5, 800.0, 800.0, 3, 0);

  // Should be improved because incomplete count decreased
  REQUIRE(result.isImproved());
}

TEST_CASE("ItemRouteResult - Comparison operator", "[autoroute][batch][itemresult]") {
  ItemRouteResult better(1, 10, 5, 1000.0, 800.0, 0, 0);
  ItemRouteResult worse(2, 10, 10, 1000.0, 1000.0, 0, 0);

  REQUIRE(better < worse);
  REQUIRE_FALSE(worse < better);
}

// ============================================================================
// Connection Tests
// ============================================================================

TEST_CASE("Connection - Construction and getters", "[autoroute][batch][connection]") {
  IntPoint start(0, 0);
  IntPoint end(1000, 1000);
  std::vector<Item*> items;  // Empty for test

  Connection conn(start, 0, end, 1, items);

  REQUIRE(conn.getStartPoint() == start);
  REQUIRE(conn.getEndPoint() == end);
  REQUIRE(conn.getStartLayer() == 0);
  REQUIRE(conn.getEndLayer() == 1);
  REQUIRE(conn.itemCount() == 0);
}

TEST_CASE("Connection - Detour calculation", "[autoroute][batch][connection]") {
  IntPoint start(0, 0);
  IntPoint end(1000, 0);
  std::vector<Item*> items;

  Connection conn(start, 0, end, 0, items);

  // With zero trace length, detour should still be calculated
  double detour = conn.getDetour();
  REQUIRE(detour > 0.0);
}

TEST_CASE("Connection - Optional endpoints", "[autoroute][batch][connection]") {
  IntPoint start(0, 0);
  IntPoint end(1000, 1000);
  std::vector<Item*> items;

  // Connection without start point
  Connection conn(false, start, 0, true, end, 1, items);

  REQUIRE_FALSE(conn.hasStart());
  REQUIRE(conn.hasEnd());
}

// ============================================================================
// ItemAutorouteInfo Tests
// ============================================================================

TEST_CASE("ItemAutorouteInfo - Construction", "[autoroute][batch][iteminfo]") {
  ItemAutorouteInfo info;

  REQUIRE(info.getPrecalculatedConnection() == nullptr);
  REQUIRE_FALSE(info.isStartInfo());
}

TEST_CASE("ItemAutorouteInfo - Start info flag", "[autoroute][batch][iteminfo]") {
  ItemAutorouteInfo info;

  info.setStartInfo(true);
  REQUIRE(info.isStartInfo());

  info.setStartInfo(false);
  REQUIRE_FALSE(info.isStartInfo());
}

TEST_CASE("ItemAutorouteInfo - Clear", "[autoroute][batch][iteminfo]") {
  ItemAutorouteInfo info;

  info.setStartInfo(true);
  info.clear();

  REQUIRE_FALSE(info.isStartInfo());
  REQUIRE(info.getPrecalculatedConnection() == nullptr);
}

// ============================================================================
// BatchAutorouter Tests
// ============================================================================

TEST_CASE("BatchAutorouter - Construction", "[autoroute][batch][router]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));
  layers.addLayer(Layer("B.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard board(layers, clearanceMatrix);

  BatchAutorouter::Config config;
  config.maxPasses = 50;
  config.startRipupCosts = 100;

  BatchAutorouter router(&board, config);

  REQUIRE(router.getCurrentPass() == 0);
}

TEST_CASE("BatchAutorouter - Single pass with no items", "[autoroute][batch][router]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard board(layers, clearanceMatrix);

  BatchAutorouter router(&board);
  SimpleStoppable stoppable;

  // With no items to route, should return false (no more items)
  bool hasMoreItems = router.autoroutePass(1, &stoppable);

  REQUIRE_FALSE(hasMoreItems);
  REQUIRE(router.getLastPassStats().itemsQueued == 0);
}

TEST_CASE("BatchAutorouter - Batch loop with no items", "[autoroute][batch][router]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard board(layers, clearanceMatrix);

  BatchAutorouter router(&board);
  SimpleStoppable stoppable;

  // With no items, should complete immediately
  bool fullyRouted = router.runBatchLoop(&stoppable);

  REQUIRE(fullyRouted);
  REQUIRE(router.getCurrentPass() == 1);
}

TEST_CASE("BatchAutorouter - Stop requested", "[autoroute][batch][router]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard board(layers, clearanceMatrix);

  BatchAutorouter router(&board);
  SimpleStoppable stoppable;

  // Request stop before starting
  stoppable.requestStop();

  bool fullyRouted = router.runBatchLoop(&stoppable);

  // Should stop immediately without completing any passes
  // fullyRouted is false because we stopped, not because routing completed
  REQUIRE_FALSE(fullyRouted);
  REQUIRE(router.getCurrentPass() == 0);
}

TEST_CASE("BatchAutorouter - Max passes limit", "[autoroute][batch][router]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  RoutingBoard board(layers, clearanceMatrix);

  BatchAutorouter::Config config;
  config.maxPasses = 5;

  BatchAutorouter router(&board, config);
  SimpleStoppable stoppable;

  // Should stop at max passes even if items remain
  // (though in this case there are no items)
  router.runBatchLoop(&stoppable);

  REQUIRE(router.getCurrentPass() <= config.maxPasses);
}
