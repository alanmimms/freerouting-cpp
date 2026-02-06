#include <catch2/catch_test_macros.hpp>
#include "board/FixedState.h"
#include "board/Item.h"
#include "board/Pin.h"
#include "board/Via.h"
#include "board/Trace.h"
#include "board/BasicBoard.h"
#include "board/LayerStructure.h"
#include "rules/ClearanceMatrix.h"
#include "core/Padstack.h"

using namespace freerouting;

TEST_CASE("FixedState enum operations", "[board][fixedstate]") {
  SECTION("Can move checks") {
    REQUIRE(canMove(FixedState::NotFixed) == true);
    REQUIRE(canMove(FixedState::ShoveFix) == false);
    REQUIRE(canMove(FixedState::UserFixed) == false);
    REQUIRE(canMove(FixedState::SystemFixed) == false);
  }

  SECTION("Can shove checks") {
    REQUIRE(canShove(FixedState::NotFixed) == true);
    REQUIRE(canShove(FixedState::ShoveFix) == true);
    REQUIRE(canShove(FixedState::UserFixed) == false);
    REQUIRE(canShove(FixedState::SystemFixed) == false);
  }

  SECTION("User fixed checks") {
    REQUIRE(isUserFixed(FixedState::NotFixed) == false);
    REQUIRE(isUserFixed(FixedState::ShoveFix) == false);
    REQUIRE(isUserFixed(FixedState::UserFixed) == true);
    REQUIRE(isUserFixed(FixedState::SystemFixed) == true);
  }

  SECTION("Max fixed state") {
    REQUIRE(maxFixedState(FixedState::NotFixed, FixedState::UserFixed) == FixedState::UserFixed);
    REQUIRE(maxFixedState(FixedState::ShoveFix, FixedState::NotFixed) == FixedState::ShoveFix);
    REQUIRE(maxFixedState(FixedState::SystemFixed, FixedState::UserFixed) == FixedState::SystemFixed);
  }
}

TEST_CASE("Pin creation and properties", "[board][pin]") {
  LayerStructure layers = {
    Layer("F.Cu", true),
    Layer("B.Cu", true)
  };

  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);
  BasicBoard board(layers, clearance);

  Padstack padstack("test_pad", 1, 0, 1, true, false);

  SECTION("Pin construction") {
    std::vector<int> nets = {1};
    Pin pin(IntPoint(100, 200), 0, &padstack, nets, 1, 1, 1,
            FixedState::NotFixed, &board);

    REQUIRE(pin.getId() == 1);
    REQUIRE(pin.getCenter().x == 100);
    REQUIRE(pin.getCenter().y == 200);
    REQUIRE(pin.getPinNumber() == 0);
    REQUIRE(pin.containsNet(1) == true);
    REQUIRE(pin.containsNet(2) == false);
    REQUIRE(pin.getComponentNumber() == 1);
  }

  SECTION("Pin is not routable") {
    std::vector<int> nets = {1};
    Pin pin(IntPoint(100, 200), 0, &padstack, nets, 1, 1, 1,
            FixedState::NotFixed, &board);

    REQUIRE(pin.isRoutable() == false);
  }

  SECTION("Pin obstacle detection") {
    std::vector<int> nets1 = {1};
    std::vector<int> nets2 = {2};

    Pin pin1(IntPoint(100, 200), 0, &padstack, nets1, 1, 1, 1,
             FixedState::NotFixed, &board);
    Pin pin2(IntPoint(150, 200), 1, &padstack, nets1, 1, 2, 1,
             FixedState::NotFixed, &board);
    Pin pin3(IntPoint(200, 200), 2, &padstack, nets2, 1, 3, 1,
             FixedState::NotFixed, &board);

    // Same pin is not obstacle
    REQUIRE(pin1.isObstacle(pin1) == false);

    // Same net pins are not obstacles
    REQUIRE(pin1.isObstacle(pin2) == false);

    // Different net pins are obstacles
    REQUIRE(pin1.isObstacle(pin3) == true);
  }

  SECTION("Pin layer range") {
    std::vector<int> nets = {1};
    Pin pin(IntPoint(100, 200), 0, &padstack, nets, 1, 1, 1,
            FixedState::NotFixed, &board);

    REQUIRE(pin.firstLayer() == 0);
    REQUIRE(pin.lastLayer() == 1);
    REQUIRE(pin.layerCount() == 2);
    REQUIRE(pin.isOnLayer(0) == true);
    REQUIRE(pin.isOnLayer(1) == true);
    REQUIRE(pin.isOnLayer(2) == false);
  }
}

TEST_CASE("Via creation and properties", "[board][via]") {
  LayerStructure layers = {
    Layer("F.Cu", true),
    Layer("In1.Cu", true),
    Layer("B.Cu", true)
  };

  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);
  BasicBoard board(layers, clearance);

  Padstack padstack("via_std", 1, 0, 2, true, false);

  SECTION("Via construction") {
    std::vector<int> nets = {1};
    Via via(IntPoint(100, 200), &padstack, nets, 1, 1,
            FixedState::NotFixed, true, &board);

    REQUIRE(via.getId() == 1);
    REQUIRE(via.getCenter().x == 100);
    REQUIRE(via.getCenter().y == 200);
    REQUIRE(via.containsNet(1) == true);
    REQUIRE(via.isAttachAllowed() == true);
    REQUIRE(via.getComponentNumber() == 0);  // Vias don't belong to components
  }

  SECTION("Via is routable when not fixed") {
    std::vector<int> nets = {1};
    Via via1(IntPoint(100, 200), &padstack, nets, 1, 1,
             FixedState::NotFixed, true, &board);
    Via via2(IntPoint(150, 200), &padstack, nets, 1, 2,
             FixedState::UserFixed, true, &board);

    REQUIRE(via1.isRoutable() == true);
    REQUIRE(via2.isRoutable() == false);
  }

  SECTION("Via obstacle detection") {
    std::vector<int> nets1 = {1};
    std::vector<int> nets2 = {2};

    Via via1(IntPoint(100, 200), &padstack, nets1, 1, 1,
             FixedState::NotFixed, true, &board);
    Via via2(IntPoint(150, 200), &padstack, nets1, 1, 2,
             FixedState::NotFixed, true, &board);
    Via via3(IntPoint(200, 200), &padstack, nets2, 1, 3,
             FixedState::NotFixed, true, &board);

    // Same via is not obstacle
    REQUIRE(via1.isObstacle(via1) == false);

    // Same net vias are not obstacles
    REQUIRE(via1.isObstacle(via2) == false);

    // Different net vias are obstacles
    REQUIRE(via1.isObstacle(via3) == true);
  }

  SECTION("Via layer range") {
    std::vector<int> nets = {1};
    Via via(IntPoint(100, 200), &padstack, nets, 1, 1,
            FixedState::NotFixed, true, &board);

    REQUIRE(via.firstLayer() == 0);
    REQUIRE(via.lastLayer() == 2);
    REQUIRE(via.layerCount() == 3);
    REQUIRE(via.isOnLayer(0) == true);
    REQUIRE(via.isOnLayer(1) == true);
    REQUIRE(via.isOnLayer(2) == true);
    REQUIRE(via.isOnLayer(3) == false);
  }
}

TEST_CASE("Trace creation and properties", "[board][trace]") {
  LayerStructure layers = {
    Layer("F.Cu", true),
    Layer("B.Cu", true)
  };

  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);
  BasicBoard board(layers, clearance);

  SECTION("Trace construction") {
    std::vector<int> nets = {1};
    Trace trace(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets, 1, 1,
                FixedState::NotFixed, &board);

    REQUIRE(trace.getId() == 1);
    REQUIRE(trace.getStart() == IntPoint(100, 100));
    REQUIRE(trace.getEnd() == IntPoint(200, 200));
    REQUIRE(trace.getLayer() == 0);
    REQUIRE(trace.getHalfWidth() == 125);
    REQUIRE(trace.getWidth() == 250);
  }

  SECTION("Trace is routable when not fixed") {
    std::vector<int> nets = {1};
    Trace trace1(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets, 1, 1,
                 FixedState::NotFixed, &board);
    Trace trace2(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets, 1, 2,
                 FixedState::UserFixed, &board);

    REQUIRE(trace1.isRoutable() == true);
    REQUIRE(trace2.isRoutable() == false);
  }

  SECTION("Trace bounding box") {
    std::vector<int> nets = {1};
    Trace trace(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets, 1, 1,
                FixedState::NotFixed, &board);

    IntBox bbox = trace.getBoundingBox();
    REQUIRE(bbox.ll.x == 100 - 125);
    REQUIRE(bbox.ll.y == 100 - 125);
    REQUIRE(bbox.ur.x == 200 + 125);
    REQUIRE(bbox.ur.y == 200 + 125);
  }

  SECTION("Trace length") {
    std::vector<int> nets = {1};
    Trace trace(IntPoint(0, 0), IntPoint(300, 400), 0, 125, nets, 1, 1,
                FixedState::NotFixed, &board);

    // 3-4-5 triangle: length should be 500
    REQUIRE(trace.getLength() == 500.0);
  }

  SECTION("Trace direction checks") {
    std::vector<int> nets = {1};

    Trace horizontal(IntPoint(100, 100), IntPoint(200, 100), 0, 125, nets, 1, 1,
                     FixedState::NotFixed, &board);
    REQUIRE(horizontal.isHorizontal() == true);
    REQUIRE(horizontal.isVertical() == false);
    REQUIRE(horizontal.isOrthogonal() == true);
    REQUIRE(horizontal.isDiagonal() == false);

    Trace vertical(IntPoint(100, 100), IntPoint(100, 200), 0, 125, nets, 1, 2,
                   FixedState::NotFixed, &board);
    REQUIRE(vertical.isHorizontal() == false);
    REQUIRE(vertical.isVertical() == true);
    REQUIRE(vertical.isOrthogonal() == true);
    REQUIRE(vertical.isDiagonal() == false);

    Trace diagonal(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets, 1, 3,
                   FixedState::NotFixed, &board);
    REQUIRE(diagonal.isHorizontal() == false);
    REQUIRE(diagonal.isVertical() == false);
    REQUIRE(diagonal.isOrthogonal() == false);
    REQUIRE(diagonal.isDiagonal() == true);
  }

  SECTION("Trace obstacle detection") {
    std::vector<int> nets1 = {1};
    std::vector<int> nets2 = {2};

    Trace trace1(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets1, 1, 1,
                 FixedState::NotFixed, &board);
    Trace trace2(IntPoint(150, 150), IntPoint(250, 250), 0, 125, nets1, 1, 2,
                 FixedState::NotFixed, &board);
    Trace trace3(IntPoint(200, 200), IntPoint(300, 300), 0, 125, nets2, 1, 3,
                 FixedState::NotFixed, &board);

    // Same trace is not obstacle
    REQUIRE(trace1.isObstacle(trace1) == false);

    // Same net traces are not obstacles
    REQUIRE(trace1.isObstacle(trace2) == false);

    // Different net traces are obstacles
    REQUIRE(trace1.isObstacle(trace3) == true);
  }

  SECTION("Trace layer range") {
    std::vector<int> nets = {1};
    Trace trace(IntPoint(100, 100), IntPoint(200, 200), 0, 125, nets, 1, 1,
                FixedState::NotFixed, &board);

    REQUIRE(trace.firstLayer() == 0);
    REQUIRE(trace.lastLayer() == 0);
    REQUIRE(trace.layerCount() == 1);
    REQUIRE(trace.isOnLayer(0) == true);
    REQUIRE(trace.isOnLayer(1) == false);
  }
}

TEST_CASE("BasicBoard item management", "[board][basicboard]") {
  LayerStructure layers = {
    Layer("F.Cu", true),
    Layer("B.Cu", true)
  };

  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);

  SECTION("Board construction") {
    BasicBoard board(layers, clearance);

    REQUIRE(board.getLayers().count() == 2);
    REQUIRE(board.itemCount() == 0);
  }

  SECTION("Add and remove items") {
    BasicBoard board(layers, clearance);
    Padstack padstack("test", 1, 0, 1);

    std::vector<int> nets = {1};
    auto pin = std::make_unique<Pin>(IntPoint(100, 100), 0, &padstack, nets, 1,
                                     board.generateItemId(), 1,
                                     FixedState::NotFixed, &board);
    int pinId = pin->getId();

    board.addItem(std::move(pin));
    REQUIRE(board.itemCount() == 1);
    REQUIRE(board.getItem(pinId) != nullptr);

    board.removeItem(pinId);
    REQUIRE(board.itemCount() == 0);
    REQUIRE(board.getItem(pinId) == nullptr);
  }

  SECTION("Get items by net") {
    BasicBoard board(layers, clearance);
    Padstack padstack("test", 1, 0, 1);

    std::vector<int> nets1 = {1};
    std::vector<int> nets2 = {2};

    board.addItem(std::make_unique<Pin>(IntPoint(100, 100), 0, &padstack, nets1, 1,
                                        board.generateItemId(), 1,
                                        FixedState::NotFixed, &board));
    board.addItem(std::make_unique<Pin>(IntPoint(200, 200), 1, &padstack, nets1, 1,
                                        board.generateItemId(), 1,
                                        FixedState::NotFixed, &board));
    board.addItem(std::make_unique<Pin>(IntPoint(300, 300), 2, &padstack, nets2, 1,
                                        board.generateItemId(), 1,
                                        FixedState::NotFixed, &board));

    auto net1Items = board.getItemsByNet(1);
    auto net2Items = board.getItemsByNet(2);

    REQUIRE(net1Items.size() == 2);
    REQUIRE(net2Items.size() == 1);
  }

  SECTION("Get items by layer") {
    BasicBoard board(layers, clearance);

    std::vector<int> nets = {1};
    board.addItem(std::make_unique<Trace>(IntPoint(0, 0), IntPoint(100, 100),
                                          0, 125, nets, 1,
                                          board.generateItemId(),
                                          FixedState::NotFixed, &board));
    board.addItem(std::make_unique<Trace>(IntPoint(100, 100), IntPoint(200, 200),
                                          1, 125, nets, 1,
                                          board.generateItemId(),
                                          FixedState::NotFixed, &board));
    board.addItem(std::make_unique<Trace>(IntPoint(200, 200), IntPoint(300, 300),
                                          0, 125, nets, 1,
                                          board.generateItemId(),
                                          FixedState::NotFixed, &board));

    auto layer0Items = board.getItemsByLayer(0);
    auto layer1Items = board.getItemsByLayer(1);

    REQUIRE(layer0Items.size() == 2);
    REQUIRE(layer1Items.size() == 1);
  }

  SECTION("ID generation") {
    BasicBoard board(layers, clearance);

    int id1 = board.generateItemId();
    int id2 = board.generateItemId();
    int id3 = board.generateItemId();

    REQUIRE(id1 == 1);
    REQUIRE(id2 == 2);
    REQUIRE(id3 == 3);
  }

  SECTION("Clear board") {
    BasicBoard board(layers, clearance);
    Padstack padstack("test", 1, 0, 1);

    std::vector<int> nets = {1};
    board.addItem(std::make_unique<Pin>(IntPoint(100, 100), 0, &padstack, nets, 1,
                                        board.generateItemId(), 1,
                                        FixedState::NotFixed, &board));
    board.addItem(std::make_unique<Pin>(IntPoint(200, 200), 1, &padstack, nets, 1,
                                        board.generateItemId(), 1,
                                        FixedState::NotFixed, &board));

    REQUIRE(board.itemCount() == 2);

    board.clear();
    REQUIRE(board.itemCount() == 0);
  }
}
