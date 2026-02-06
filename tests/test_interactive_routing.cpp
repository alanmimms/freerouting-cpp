#include <catch2/catch_test_macros.hpp>
#include "board/Trace.h"
#include "board/Via.h"
#include "board/RouteOptimizer.h"
#include "board/FixedState.h"
#include "core/Padstack.h"
#include <cmath>

using namespace freerouting;

// ============================================================================
// Trace Tests
// ============================================================================

TEST_CASE("Trace - Construction and basic properties", "[routing][trace]") {
  IntPoint start(0, 0);
  IntPoint end(100, 0);
  std::vector<int> nets = {1};

  Trace trace(start, end, 0, 50, nets, 0, 1, FixedState::NotFixed, nullptr);

  REQUIRE(trace.getStart() == start);
  REQUIRE(trace.getEnd() == end);
  REQUIRE(trace.getLayer() == 0);
  REQUIRE(trace.getHalfWidth() == 50);
  REQUIRE(trace.getWidth() == 100);
  REQUIRE(trace.getNets().size() == 1);
  REQUIRE(trace.getNets()[0] == 1);
}

TEST_CASE("Trace - Bounding box", "[routing][trace]") {
  IntPoint start(0, 0);
  IntPoint end(100, 50);
  Trace trace(start, end, 0, 25, {1}, 0, 1, FixedState::NotFixed, nullptr);

  IntBox bbox = trace.getBoundingBox();

  // Bounding box should include half-width on all sides
  REQUIRE(bbox.ll.x == -25);  // min(0, 100) - 25
  REQUIRE(bbox.ll.y == -25);  // min(0, 50) - 25
  REQUIRE(bbox.ur.x == 125);  // max(0, 100) + 25
  REQUIRE(bbox.ur.y == 75);   // max(0, 50) + 25
}

TEST_CASE("Trace - Length calculation", "[routing][trace]") {
  // Horizontal trace
  Trace horizTrace(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
                   FixedState::NotFixed, nullptr);
  REQUIRE(horizTrace.getLength() == 100.0);

  // Vertical trace
  Trace vertTrace(IntPoint(0, 0), IntPoint(0, 50), 0, 10, {1}, 0, 2,
                  FixedState::NotFixed, nullptr);
  REQUIRE(vertTrace.getLength() == 50.0);

  // Diagonal trace (3-4-5 triangle)
  Trace diagTrace(IntPoint(0, 0), IntPoint(3, 4), 0, 10, {1}, 0, 3,
                  FixedState::NotFixed, nullptr);
  REQUIRE(diagTrace.getLength() == 5.0);
}

TEST_CASE("Trace - Orientation checks", "[routing][trace]") {
  Trace horiz(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
              FixedState::NotFixed, nullptr);
  REQUIRE(horiz.isHorizontal());
  REQUIRE_FALSE(horiz.isVertical());
  REQUIRE(horiz.isOrthogonal());
  REQUIRE_FALSE(horiz.isDiagonal());

  Trace vert(IntPoint(0, 0), IntPoint(0, 100), 0, 10, {1}, 0, 2,
             FixedState::NotFixed, nullptr);
  REQUIRE_FALSE(vert.isHorizontal());
  REQUIRE(vert.isVertical());
  REQUIRE(vert.isOrthogonal());
  REQUIRE_FALSE(vert.isDiagonal());

  Trace diag(IntPoint(0, 0), IntPoint(100, 100), 0, 10, {1}, 0, 3,
             FixedState::NotFixed, nullptr);
  REQUIRE_FALSE(diag.isHorizontal());
  REQUIRE_FALSE(diag.isVertical());
  REQUIRE_FALSE(diag.isOrthogonal());
  REQUIRE(diag.isDiagonal());
}

TEST_CASE("Trace - Obstacle detection", "[routing][trace]") {
  Trace trace1(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
               FixedState::NotFixed, nullptr);
  Trace trace2(IntPoint(0, 50), IntPoint(100, 50), 0, 10, {2}, 0, 2,
               FixedState::NotFixed, nullptr);
  Trace trace3(IntPoint(200, 0), IntPoint(300, 0), 0, 10, {1}, 0, 3,
               FixedState::NotFixed, nullptr);

  // Different nets = obstacles
  REQUIRE(trace1.isObstacle(trace2));
  REQUIRE(trace2.isObstacle(trace1));

  // Same net = not obstacles
  REQUIRE_FALSE(trace1.isObstacle(trace3));
  REQUIRE_FALSE(trace3.isObstacle(trace1));
}

TEST_CASE("Trace - Routable check", "[routing][trace]") {
  Trace routable(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
                 FixedState::NotFixed, nullptr);
  REQUIRE(routable.isRoutable());

  Trace fixed(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 2,
              FixedState::UserFixed, nullptr);
  REQUIRE_FALSE(fixed.isRoutable());

  Trace noNet(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {}, 0, 3,
              FixedState::NotFixed, nullptr);
  REQUIRE_FALSE(noNet.isRoutable());
}

TEST_CASE("Trace - Copy", "[routing][trace]") {
  Trace original(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
                 FixedState::NotFixed, nullptr);

  Item* copy = original.copy(99);
  REQUIRE(copy != nullptr);
  REQUIRE(copy->getId() == 99);

  auto* traceCopy = dynamic_cast<Trace*>(copy);
  REQUIRE(traceCopy != nullptr);
  REQUIRE(traceCopy->getStart() == IntPoint(0, 0));
  REQUIRE(traceCopy->getEnd() == IntPoint(100, 0));
  REQUIRE(traceCopy->getHalfWidth() == 50);

  delete copy;
}

// ============================================================================
// Via Tests
// ============================================================================

TEST_CASE("Via - Construction and basic properties", "[routing][via]") {
  IntPoint center(100, 100);
  std::vector<int> nets = {1};

  // Note: Padstack pointer can be nullptr for testing
  Via via(center, nullptr, nets, 0, 1, FixedState::NotFixed, true, nullptr);

  REQUIRE(via.getCenter() == center);
  REQUIRE(via.getNets().size() == 1);
  REQUIRE(via.getNets()[0] == 1);
  REQUIRE(via.isAttachAllowed());
}

TEST_CASE("Via - Routable check", "[routing][via]") {
  Via routable(IntPoint(0, 0), nullptr, {1}, 0, 1,
               FixedState::NotFixed, true, nullptr);
  REQUIRE(routable.isRoutable());

  Via fixed(IntPoint(0, 0), nullptr, {1}, 0, 2,
            FixedState::UserFixed, true, nullptr);
  REQUIRE_FALSE(fixed.isRoutable());

  Via noNet(IntPoint(0, 0), nullptr, {}, 0, 3,
            FixedState::NotFixed, true, nullptr);
  REQUIRE_FALSE(noNet.isRoutable());
}

TEST_CASE("Via - Obstacle detection", "[routing][via]") {
  Via via1(IntPoint(0, 0), nullptr, {1}, 0, 1,
           FixedState::NotFixed, true, nullptr);
  Via via2(IntPoint(100, 100), nullptr, {2}, 0, 2,
           FixedState::NotFixed, true, nullptr);
  Via via3(IntPoint(200, 200), nullptr, {1}, 0, 3,
           FixedState::NotFixed, true, nullptr);

  // Different nets = obstacles
  REQUIRE(via1.isObstacle(via2));
  REQUIRE(via2.isObstacle(via1));

  // Same net = not obstacles
  REQUIRE_FALSE(via1.isObstacle(via3));
  REQUIRE_FALSE(via3.isObstacle(via1));
}

TEST_CASE("Via - Copy", "[routing][via]") {
  Via original(IntPoint(100, 100), nullptr, {1}, 0, 1,
               FixedState::NotFixed, true, nullptr);

  Item* copy = original.copy(99);
  REQUIRE(copy != nullptr);
  REQUIRE(copy->getId() == 99);

  auto* viaCopy = dynamic_cast<Via*>(copy);
  REQUIRE(viaCopy != nullptr);
  REQUIRE(viaCopy->getCenter() == IntPoint(100, 100));
  REQUIRE(viaCopy->isAttachAllowed());

  delete copy;
}

// ============================================================================
// RouteOptimizer Tests
// ============================================================================

TEST_CASE("RouteOptimizer - Merge collinear traces", "[routing][optimizer]") {
  // Two collinear traces that share an endpoint
  Trace trace1(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
               FixedState::NotFixed, nullptr);
  Trace trace2(IntPoint(100, 0), IntPoint(200, 0), 0, 50, {1}, 0, 2,
               FixedState::NotFixed, nullptr);

  Trace* merged = RouteOptimizer::mergeCollinearTraces(trace1, trace2);
  REQUIRE(merged != nullptr);

  REQUIRE(merged->getStart() == IntPoint(0, 0));
  REQUIRE(merged->getEnd() == IntPoint(200, 0));
  REQUIRE(merged->getHalfWidth() == 50);

  delete merged;
}

TEST_CASE("RouteOptimizer - Cannot merge non-collinear traces", "[routing][optimizer]") {
  // Two traces at right angle
  Trace trace1(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
               FixedState::NotFixed, nullptr);
  Trace trace2(IntPoint(100, 0), IntPoint(100, 100), 0, 50, {1}, 0, 2,
               FixedState::NotFixed, nullptr);

  Trace* merged = RouteOptimizer::mergeCollinearTraces(trace1, trace2);
  REQUIRE(merged == nullptr);
}

TEST_CASE("RouteOptimizer - Cannot merge traces with different widths", "[routing][optimizer]") {
  Trace trace1(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
               FixedState::NotFixed, nullptr);
  Trace trace2(IntPoint(100, 0), IntPoint(200, 0), 0, 25, {1}, 0, 2,
               FixedState::NotFixed, nullptr);

  Trace* merged = RouteOptimizer::mergeCollinearTraces(trace1, trace2);
  REQUIRE(merged == nullptr);
}

TEST_CASE("RouteOptimizer - Point on segment check", "[routing][optimizer]") {
  IntPoint segStart(0, 0);
  IntPoint segEnd(100, 0);

  // Point on segment
  REQUIRE(RouteOptimizer::isPointOnSegment(IntPoint(50, 0), segStart, segEnd));

  // Endpoints
  REQUIRE(RouteOptimizer::isPointOnSegment(segStart, segStart, segEnd));
  REQUIRE(RouteOptimizer::isPointOnSegment(segEnd, segStart, segEnd));

  // Point not on segment (wrong y)
  REQUIRE_FALSE(RouteOptimizer::isPointOnSegment(IntPoint(50, 10), segStart, segEnd));

  // Point on line but outside segment
  REQUIRE_FALSE(RouteOptimizer::isPointOnSegment(IntPoint(150, 0), segStart, segEnd));
  REQUIRE_FALSE(RouteOptimizer::isPointOnSegment(IntPoint(-50, 0), segStart, segEnd));
}

TEST_CASE("RouteOptimizer - Calculate route length", "[routing][optimizer]") {
  std::vector<Trace*> traces;
  traces.push_back(new Trace(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(100, 0), IntPoint(100, 50), 0, 10, {1}, 0, 2,
                              FixedState::NotFixed, nullptr));

  double length = RouteOptimizer::calculateRouteLength(traces);
  REQUIRE(length == 150.0);

  for (auto* t : traces) delete t;
}

TEST_CASE("RouteOptimizer - Count direction changes", "[routing][optimizer]") {
  std::vector<Trace*> traces;

  // Straight route (no direction changes)
  traces.push_back(new Trace(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(100, 0), IntPoint(200, 0), 0, 10, {1}, 0, 2,
                              FixedState::NotFixed, nullptr));

  REQUIRE(RouteOptimizer::countDirectionChanges(traces) == 0);

  // Add a right-angle turn
  traces.push_back(new Trace(IntPoint(200, 0), IntPoint(200, 100), 0, 10, {1}, 0, 3,
                              FixedState::NotFixed, nullptr));

  REQUIRE(RouteOptimizer::countDirectionChanges(traces) == 1);

  for (auto* t : traces) delete t;
}

TEST_CASE("RouteOptimizer - Find mergeable trace pairs", "[routing][optimizer]") {
  std::vector<Trace*> traces;

  // Two mergeable traces
  traces.push_back(new Trace(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(100, 0), IntPoint(200, 0), 0, 50, {1}, 0, 2,
                              FixedState::NotFixed, nullptr));

  // One non-mergeable trace (different direction)
  traces.push_back(new Trace(IntPoint(200, 0), IntPoint(200, 100), 0, 50, {1}, 0, 3,
                              FixedState::NotFixed, nullptr));

  auto pairs = RouteOptimizer::findMergeableTracePairs(traces);

  REQUIRE(pairs.size() == 1);
  REQUIRE(pairs[0].first == 0);
  REQUIRE(pairs[0].second == 1);

  for (auto* t : traces) delete t;
}

TEST_CASE("RouteOptimizer - Wire length metric", "[routing][optimizer]") {
  std::vector<Trace*> traces;

  // Simple straight route
  traces.push_back(new Trace(IntPoint(0, 0), IntPoint(100, 0), 0, 10, {1}, 0, 1,
                              FixedState::NotFixed, nullptr));

  double metric1 = RouteOptimizer::calculateWireLengthMetric(traces);
  REQUIRE(metric1 == 100.0);  // Just the length, no bends

  // Add a bend
  traces.push_back(new Trace(IntPoint(100, 0), IntPoint(100, 50), 0, 10, {1}, 0, 2,
                              FixedState::NotFixed, nullptr));

  double metric2 = RouteOptimizer::calculateWireLengthMetric(traces);
  REQUIRE(metric2 == 250.0);  // 150 length + 100 bend penalty

  for (auto* t : traces) delete t;
}

TEST_CASE("RouteOptimizer - Straighten simple route", "[routing][optimizer]") {
  std::vector<Trace*> traces;

  // Three collinear traces that can be merged into one
  traces.push_back(new Trace(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(100, 0), IntPoint(200, 0), 0, 50, {1}, 0, 2,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(200, 0), IntPoint(300, 0), 0, 50, {1}, 0, 3,
                              FixedState::NotFixed, nullptr));

  auto optimized = RouteOptimizer::straightenRoute(traces);

  REQUIRE(optimized.size() == 1);
  REQUIRE(optimized[0]->getStart() == IntPoint(0, 0));
  REQUIRE(optimized[0]->getEnd() == IntPoint(300, 0));

  for (auto* t : traces) delete t;
  for (auto* t : optimized) delete t;
}

TEST_CASE("RouteOptimizer - Straighten route with bends", "[routing][optimizer]") {
  std::vector<Trace*> traces;

  // L-shaped route (cannot be fully straightened)
  traces.push_back(new Trace(IntPoint(0, 0), IntPoint(100, 0), 0, 50, {1}, 0, 1,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(100, 0), IntPoint(200, 0), 0, 50, {1}, 0, 2,
                              FixedState::NotFixed, nullptr));
  traces.push_back(new Trace(IntPoint(200, 0), IntPoint(200, 100), 0, 50, {1}, 0, 3,
                              FixedState::NotFixed, nullptr));

  auto optimized = RouteOptimizer::straightenRoute(traces);

  REQUIRE(optimized.size() == 2);  // Should combine first two, leave the bend

  for (auto* t : traces) delete t;
  for (auto* t : optimized) delete t;
}
