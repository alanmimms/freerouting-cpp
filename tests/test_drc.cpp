#include <catch2/catch_test_macros.hpp>
#include "board/DrcEngine.h"
#include "board/DrcViolation.h"
#include "board/BasicBoard.h"
#include "board/RuleArea.h"
#include "board/LayerStructure.h"
#include "rules/ClearanceMatrix.h"
#include "board/Item.h"

using namespace freerouting;

// ============================================================================
// DrcViolation Tests
// ============================================================================

TEST_CASE("DrcViolation - Construction and basic properties", "[drc][violation]") {
  DrcViolation violation(
    DrcViolation::Type::Clearance,
    DrcViolation::Severity::Error,
    "Test clearance violation"
  );

  REQUIRE(violation.getType() == DrcViolation::Type::Clearance);
  REQUIRE(violation.getSeverity() == DrcViolation::Severity::Error);
  REQUIRE(violation.getMessage() == "Test clearance violation");
}

TEST_CASE("DrcViolation - Setting properties", "[drc][violation]") {
  DrcViolation violation(
    DrcViolation::Type::RuleArea,
    DrcViolation::Severity::Warning,
    "Test rule area violation"
  );

  violation.setLayer(2);
  violation.setLocation(IntPoint(100, 200));
  violation.addItemId(42);
  violation.addItemId(99);

  REQUIRE(violation.getLayer() == 2);
  REQUIRE(violation.getLocation() == IntPoint(100, 200));
  REQUIRE(violation.getItemIds().size() == 2);
  REQUIRE(violation.getItemIds()[0] == 42);
  REQUIRE(violation.getItemIds()[1] == 99);
}

TEST_CASE("DrcViolation - String formatting", "[drc][violation]") {
  DrcViolation errorViolation(
    DrcViolation::Type::Clearance,
    DrcViolation::Severity::Error,
    "Insufficient clearance"
  );

  std::string str = errorViolation.toString();
  REQUIRE(str.find("ERROR") != std::string::npos);
  REQUIRE(str.find("Clearance Violation") != std::string::npos);
  REQUIRE(str.find("Insufficient clearance") != std::string::npos);

  DrcViolation warningViolation(
    DrcViolation::Type::TraceWidth,
    DrcViolation::Severity::Warning,
    "Narrow trace"
  );

  str = warningViolation.toString();
  REQUIRE(str.find("WARNING") != std::string::npos);
  REQUIRE(str.find("Trace Width Violation") != std::string::npos);
}

TEST_CASE("DrcViolation - Type to string conversion", "[drc][violation]") {
  REQUIRE(DrcViolation::typeToString(DrcViolation::Type::Clearance) ==
          "Clearance Violation");
  REQUIRE(DrcViolation::typeToString(DrcViolation::Type::TraceWidth) ==
          "Trace Width Violation");
  REQUIRE(DrcViolation::typeToString(DrcViolation::Type::RuleArea) ==
          "Rule Area Violation");
  REQUIRE(DrcViolation::typeToString(DrcViolation::Type::NetConflict) ==
          "Net Conflict");
}

// ============================================================================
// DrcEngine Tests
// ============================================================================

// Simple test item for DRC testing
class TestItem : public Item {
public:
  TestItem(int id, int layer, IntBox box, int netNo = 0)
    : Item((netNo == 0) ? std::vector<int>{} : std::vector<int>{netNo},
           0,           // clearanceClass
           id,
           0,           // componentNumber
           FixedState::NotFixed,
           nullptr),    // board
      box_(box),
      layer_(layer) {}

  IntBox getBoundingBox() const override { return box_; }

  int firstLayer() const override { return layer_; }
  int lastLayer() const override { return layer_; }

  bool isObstacle(const Item& other) const override {
    // Simple obstacle check - items on same layer with different nets
    if (!isOnLayer(other.firstLayer())) {
      return false;
    }
    return !sharesNet(other);
  }

  Item* copy(int newId) const override {
    int netNo = getNets().empty() ? 0 : getNets()[0];
    return new TestItem(newId, layer_, box_, netNo);
  }

private:
  IntBox box_;
  int layer_;
};

TEST_CASE("DrcEngine - Construction", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  DrcEngine engine(&board);

  REQUIRE(engine.getErrorCount() == 0);
  REQUIRE(engine.getWarningCount() == 0);
}

TEST_CASE("DrcEngine - No violations on empty board", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  DrcEngine engine(&board);
  auto violations = engine.checkAll();

  REQUIRE(violations.empty());
  REQUIRE(engine.getErrorCount() == 0);
  REQUIRE(engine.getWarningCount() == 0);
}

TEST_CASE("DrcEngine - No violations with well-spaced items", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  // Add two items with plenty of clearance
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(500, 500, 600, 600), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkClearances();

  REQUIRE(violations.empty());
}

TEST_CASE("DrcEngine - Detect clearance violation", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);

  // Set a clearance requirement of 100 units
  clearanceMatrix.setValue(0, 0, 0, 100);

  BasicBoard board(layers, clearanceMatrix);

  // Add two items too close together (50 units apart)
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(150, 0, 250, 100), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkClearances();

  REQUIRE(violations.size() == 1);
  REQUIRE(violations[0].getType() == DrcViolation::Type::Clearance);
  REQUIRE(violations[0].getSeverity() == DrcViolation::Severity::Error);
  REQUIRE(violations[0].getItemIds().size() == 2);
}

TEST_CASE("DrcEngine - Detect overlapping items", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  clearanceMatrix.setValue(0, 0, 0, 10);

  BasicBoard board(layers, clearanceMatrix);

  // Add two overlapping items
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(50, 50, 150, 150), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkClearances();

  REQUIRE_FALSE(violations.empty());
  REQUIRE(violations[0].getType() == DrcViolation::Type::Clearance);
}

TEST_CASE("DrcEngine - Items on different layers don't conflict", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));
  layers.addLayer(Layer("B.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  clearanceMatrix.setValue(0, 0, 0, 100);
  clearanceMatrix.setValue(0, 0, 1, 100);

  BasicBoard board(layers, clearanceMatrix);

  // Add two items on different layers at same location
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 1, IntBox(0, 0, 100, 100), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkClearances();

  // Should have no violations since items are on different layers
  REQUIRE(violations.empty());
}

TEST_CASE("DrcEngine - Net conflict detection", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  // Add two overlapping items with different nets
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(50, 50, 150, 150), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkNetConflicts();

  REQUIRE_FALSE(violations.empty());
  REQUIRE(violations[0].getType() == DrcViolation::Type::NetConflict);
  REQUIRE(violations[0].getSeverity() == DrcViolation::Severity::Error);
}

TEST_CASE("DrcEngine - Same net items don't create net conflict", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  // Add two overlapping items with SAME net
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(50, 50, 150, 150), 1);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkNetConflicts();

  // No net conflict since items are on same net
  REQUIRE(violations.empty());
}

TEST_CASE("DrcEngine - Rule area violation detection", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  BasicBoard board(layers, clearanceMatrix);

  // Add a rule area that prohibits traces
  std::vector<int> nets = {};
  board.addRuleArea(std::make_unique<RuleArea>(
    100, 0, "keepout", false, false, false, nets));

  // Add an item (note: RuleArea::contains() is stubbed to return false)
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  board.addItem(std::move(item1));

  DrcEngine engine(&board);
  auto violations = engine.checkRuleAreas();

  // With stub implementation, no violations are detected
  // When geometry is implemented, this will detect violations
  REQUIRE(violations.empty());
}

TEST_CASE("DrcEngine - Statistics tracking", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  clearanceMatrix.setValue(0, 0, 0, 100);

  BasicBoard board(layers, clearanceMatrix);

  // Add several items with violations
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(120, 0, 220, 100), 2);
  auto item3 = std::make_unique<TestItem>(3, 0, IntBox(240, 0, 340, 100), 3);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));
  board.addItem(std::move(item3));

  DrcEngine engine(&board);
  auto violations = engine.checkAll();

  // Should have error count
  REQUIRE(engine.getErrorCount() > 0);
  REQUIRE(engine.getErrorCount() == static_cast<int>(violations.size()));
}

TEST_CASE("DrcEngine - Clear statistics", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  clearanceMatrix.setValue(0, 0, 0, 100);

  BasicBoard board(layers, clearanceMatrix);

  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(120, 0, 220, 100), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  engine.checkAll();

  REQUIRE(engine.getErrorCount() > 0);

  engine.clearStatistics();
  REQUIRE(engine.getErrorCount() == 0);
  REQUIRE(engine.getWarningCount() == 0);
}

TEST_CASE("DrcEngine - checkAll combines all violation types", "[drc][engine]") {
  LayerStructure layers;
  layers.addLayer(Layer("F.Cu", true));

  std::vector<std::string> classNames = {"default"};
  ClearanceMatrix clearanceMatrix(1, layers, classNames);
  clearanceMatrix.setValue(0, 0, 0, 100);

  BasicBoard board(layers, clearanceMatrix);

  // Add items with both clearance and net conflict violations
  auto item1 = std::make_unique<TestItem>(1, 0, IntBox(0, 0, 100, 100), 1);
  auto item2 = std::make_unique<TestItem>(2, 0, IntBox(50, 50, 150, 150), 2);

  board.addItem(std::move(item1));
  board.addItem(std::move(item2));

  DrcEngine engine(&board);
  auto violations = engine.checkAll();

  // Should detect both clearance and net conflict
  REQUIRE(violations.size() >= 2);

  bool hasClearance = false;
  bool hasNetConflict = false;

  for (const auto& v : violations) {
    if (v.getType() == DrcViolation::Type::Clearance) hasClearance = true;
    if (v.getType() == DrcViolation::Type::NetConflict) hasNetConflict = true;
  }

  REQUIRE(hasClearance);
  REQUIRE(hasNetConflict);
}
