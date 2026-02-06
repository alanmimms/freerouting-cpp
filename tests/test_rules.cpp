#include <catch2/catch_test_macros.hpp>
#include "board/Layer.h"
#include "board/LayerStructure.h"
#include "board/AngleRestriction.h"
#include "rules/ClearanceMatrix.h"
#include "rules/ViaInfo.h"
#include "rules/ViaRule.h"
#include "rules/NetClass.h"
#include "rules/Net.h"
#include "rules/Nets.h"
#include "core/Padstack.h"

using namespace freerouting;

TEST_CASE("Layer basic operations", "[rules][layer]") {
  SECTION("Layer construction") {
    Layer signal("F.Cu", true);
    REQUIRE(signal.name == "F.Cu");
    REQUIRE(signal.isSignal == true);

    Layer power("GND", false);
    REQUIRE(power.name == "GND");
    REQUIRE(power.isSignal == false);
  }

  SECTION("Default layer construction") {
    Layer layer;
    REQUIRE(layer.name == "");
    REQUIRE(layer.isSignal == false);
  }
}

TEST_CASE("LayerStructure operations", "[rules][layerstructure]") {
  SECTION("Construction from initializer list") {
    LayerStructure layers = {
      Layer("F.Cu", true),
      Layer("In1.Cu", true),
      Layer("In2.Cu", true),
      Layer("B.Cu", true)
    };

    REQUIRE(layers.count() == 4);
    REQUIRE(layers[0].name == "F.Cu");
    REQUIRE(layers[3].name == "B.Cu");
  }

  SECTION("Signal layer count") {
    LayerStructure layers = {
      Layer("F.Cu", true),
      Layer("GND", false),
      Layer("In1.Cu", true),
      Layer("PWR", false),
      Layer("B.Cu", true)
    };

    REQUIRE(layers.count() == 5);
    REQUIRE(layers.signalLayerCount() == 3);
  }

  SECTION("Layer lookup by name") {
    LayerStructure layers = {
      Layer("F.Cu", true),
      Layer("In1.Cu", true),
      Layer("B.Cu", true)
    };

    REQUIRE(layers.getLayerNumber("F.Cu") == 0);
    REQUIRE(layers.getLayerNumber("In1.Cu") == 1);
    REQUIRE(layers.getLayerNumber("B.Cu") == 2);
    REQUIRE(layers.getLayerNumber("NotFound") == -1);
  }

  SECTION("Get signal layer") {
    LayerStructure layers = {
      Layer("F.Cu", true),
      Layer("GND", false),
      Layer("In1.Cu", true),
      Layer("B.Cu", true)
    };

    const Layer& sig0 = layers.getSignalLayer(0);
    const Layer& sig1 = layers.getSignalLayer(1);
    const Layer& sig2 = layers.getSignalLayer(2);

    REQUIRE(sig0.name == "F.Cu");
    REQUIRE(sig1.name == "In1.Cu");
    REQUIRE(sig2.name == "B.Cu");
  }

  SECTION("Signal layer number") {
    LayerStructure layers = {
      Layer("F.Cu", true),
      Layer("GND", false),
      Layer("In1.Cu", true)
    };

    REQUIRE(layers.getSignalLayerNumber(layers[0]) == 0);  // F.Cu
    REQUIRE(layers.getSignalLayerNumber(layers[1]) == -1); // GND (not signal)
    REQUIRE(layers.getSignalLayerNumber(layers[2]) == 1);  // In1.Cu
  }
}

TEST_CASE("AngleRestriction enum", "[rules][angle]") {
  SECTION("Enum values") {
    REQUIRE(angleRestrictionToInt(AngleRestriction::None) == 0);
    REQUIRE(angleRestrictionToInt(AngleRestriction::FortyFiveDegree) == 1);
    REQUIRE(angleRestrictionToInt(AngleRestriction::NinetyDegree) == 2);
  }

  SECTION("Conversion from int") {
    REQUIRE(angleRestrictionFromInt(0) == AngleRestriction::None);
    REQUIRE(angleRestrictionFromInt(1) == AngleRestriction::FortyFiveDegree);
    REQUIRE(angleRestrictionFromInt(2) == AngleRestriction::NinetyDegree);
    REQUIRE(angleRestrictionFromInt(99) == AngleRestriction::None);  // Default
  }
}

TEST_CASE("ClearanceMatrix operations", "[rules][clearance]") {
  LayerStructure layers = {
    Layer("F.Cu", true),
    Layer("In1.Cu", true),
    Layer("B.Cu", true)
  };

  SECTION("Default clearance matrix") {
    auto matrix = ClearanceMatrix::createDefault(layers, 1000);

    REQUIRE(matrix.getClassCount() == 2);
    REQUIRE(matrix.getLayerCount() == 3);
    REQUIRE(matrix.getClassName(0) == "null");
    REQUIRE(matrix.getClassName(1) == "default");
  }

  SECTION("Custom clearance matrix") {
    std::vector<std::string> names = {"class0", "class1", "class2"};
    ClearanceMatrix matrix(3, layers, names);

    REQUIRE(matrix.getClassCount() == 3);
    REQUIRE(matrix.getClassName(0) == "class0");
    REQUIRE(matrix.getClassName(1) == "class1");
    REQUIRE(matrix.getClassName(2) == "class2");
  }

  SECTION("Set and get clearance values") {
    std::vector<std::string> names = {"null", "default"};
    ClearanceMatrix matrix(2, layers, names);

    matrix.setValue(1, 1, 0, 500);  // Layer 0
    matrix.setValue(1, 1, 1, 600);  // Layer 1
    matrix.setValue(1, 1, 2, 700);  // Layer 2

    REQUIRE(matrix.getValue(1, 1, 0, false) == 500);
    REQUIRE(matrix.getValue(1, 1, 1, false) == 600);
    REQUIRE(matrix.getValue(1, 1, 2, false) == 700);
  }

  SECTION("Clearance values forced to be even") {
    std::vector<std::string> names = {"null", "default"};
    ClearanceMatrix matrix(2, layers, names);

    matrix.setValue(1, 1, 0, 501);  // Odd value
    REQUIRE(matrix.getValue(1, 1, 0, false) == 502);  // Rounded up

    matrix.setValue(1, 1, 0, -10);  // Negative value
    REQUIRE(matrix.getValue(1, 1, 0, false) == 0);  // Clamped to 0
  }

  SECTION("Safety margin") {
    std::vector<std::string> names = {"null", "default"};
    ClearanceMatrix matrix(2, layers, names);

    matrix.setValue(1, 1, 0, 1000);
    REQUIRE(matrix.getValue(1, 1, 0, false) == 1000);
    REQUIRE(matrix.getValue(1, 1, 0, true) == 1000 + ClearanceMatrix::kClearanceSafetyMargin);
  }

  SECTION("Layer dependency check") {
    std::vector<std::string> names = {"null", "default"};
    ClearanceMatrix matrix(2, layers, names);

    matrix.setValue(1, 1, 0, 1000);
    matrix.setValue(1, 1, 1, 1000);
    matrix.setValue(1, 1, 2, 1000);
    REQUIRE_FALSE(matrix.isLayerDependent(1, 1));

    matrix.setValue(1, 1, 1, 2000);
    REQUIRE(matrix.isLayerDependent(1, 1));
  }
}

TEST_CASE("Padstack operations", "[rules][padstack]") {
  SECTION("Padstack construction") {
    Padstack ps("via_std", 1, 0, 3, true, false);

    REQUIRE(ps.name == "via_std");
    REQUIRE(ps.number == 1);
    REQUIRE(ps.fromLayer() == 0);
    REQUIRE(ps.toLayer() == 3);
    REQUIRE(ps.attachAllowed == true);
    REQUIRE(ps.placedAbsolute == false);
  }

  SECTION("Layer range modification") {
    Padstack ps("via_blind", 2, 0, 1);
    ps.setLayerRange(1, 3);

    REQUIRE(ps.fromLayer() == 1);
    REQUIRE(ps.toLayer() == 3);
  }
}

TEST_CASE("ViaInfo and ViaRule", "[rules][via]") {
  Padstack ps1("via_small", 1, 0, 3);
  Padstack ps2("via_large", 2, 0, 3);

  SECTION("ViaInfo construction") {
    ViaInfo via("Small Via", &ps1, 1, true);

    REQUIRE(via.getName() == "Small Via");
    REQUIRE(via.getPadstack() == &ps1);
    REQUIRE(via.getClearanceClass() == 1);
    REQUIRE(via.isAttachSmdAllowed() == true);
  }

  SECTION("ViaRule construction and operations") {
    ViaRule rule("default_rule");
    ViaInfo via1("Small", &ps1, 1, true);
    ViaInfo via2("Large", &ps2, 1, false);

    rule.appendVia(via1);
    rule.appendVia(via2);

    REQUIRE(rule.getName() == "default_rule");
    REQUIRE(rule.viaCount() == 2);
    REQUIRE(rule.getVia(0).getName() == "Small");
    REQUIRE(rule.getVia(1).getName() == "Large");
  }

  SECTION("ViaRule contains check") {
    ViaRule rule("test");
    ViaInfo via1("Via1", &ps1, 1, true);
    ViaInfo via2("Via2", &ps2, 1, true);

    rule.appendVia(via1);

    REQUIRE(rule.contains(via1) == true);
    REQUIRE(rule.contains(via2) == false);
  }

  SECTION("ViaRule layer range lookup") {
    Padstack ps_blind("blind", 3, 0, 1);
    Padstack ps_through("through", 4, 0, 3);

    ViaInfo via_blind("Blind", &ps_blind, 1, true);
    ViaInfo via_through("Through", &ps_through, 1, true);

    ViaRule rule("test");
    rule.appendVia(via_blind);
    rule.appendVia(via_through);

    const ViaInfo* found = rule.getViaForLayerRange(0, 1);
    REQUIRE(found != nullptr);
    REQUIRE(found->getName() == "Blind");

    found = rule.getViaForLayerRange(0, 3);
    REQUIRE(found != nullptr);
    REQUIRE(found->getName() == "Through");

    found = rule.getViaForLayerRange(1, 2);
    REQUIRE(found == nullptr);
  }
}

TEST_CASE("NetClass operations", "[rules][netclass]") {
  LayerStructure layers = {
    Layer("F.Cu", true),
    Layer("In1.Cu", true),
    Layer("B.Cu", true)
  };

  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);

  SECTION("NetClass construction") {
    NetClass nc("Default", layers, clearance);

    REQUIRE(nc.getName() == "Default");
    REQUIRE(nc.layerCount() == 3);
  }

  SECTION("Trace width operations") {
    NetClass nc("Test", layers, clearance);

    nc.setTraceHalfWidth(200);
    REQUIRE(nc.getTraceHalfWidth(0) == 200);
    REQUIRE(nc.getTraceHalfWidth(1) == 200);
    REQUIRE(nc.getTraceHalfWidth(2) == 200);

    nc.setTraceHalfWidth(1, 300);
    REQUIRE(nc.getTraceHalfWidth(1) == 300);
    REQUIRE_FALSE(nc.traceWidthIsLayerDependent() == false);  // Now layer-dependent
  }

  SECTION("Active routing layers") {
    NetClass nc("Test", layers, clearance);

    // All signal layers should be active by default
    REQUIRE(nc.isActiveRoutingLayer(0) == true);
    REQUIRE(nc.isActiveRoutingLayer(1) == true);
    REQUIRE(nc.isActiveRoutingLayer(2) == true);

    nc.setActiveRoutingLayer(1, false);
    REQUIRE(nc.isActiveRoutingLayer(1) == false);

    nc.setAllLayersActive(false);
    REQUIRE(nc.isActiveRoutingLayer(0) == false);
    REQUIRE(nc.isActiveRoutingLayer(2) == false);
  }

  SECTION("NetClass properties") {
    NetClass nc("Power", layers, clearance);

    nc.setTraceClearanceClass(2);
    nc.setShoveFix(true);
    nc.setPullTight(false);
    nc.setMinimumTraceLength(10.0);
    nc.setMaximumTraceLength(1000.0);

    REQUIRE(nc.getTraceClearanceClass() == 2);
    REQUIRE(nc.isShoveFix() == true);
    REQUIRE(nc.isPullTight() == false);
    REQUIRE(nc.getMinimumTraceLength() == 10.0);
    REQUIRE(nc.getMaximumTraceLength() == 1000.0);
  }
}

TEST_CASE("Net operations", "[rules][net]") {
  LayerStructure layers = {Layer("F.Cu", true), Layer("B.Cu", true)};
  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);
  NetClass defaultClass("Default", layers, clearance);

  SECTION("Net construction") {
    Net net("GND", 1, 100, &defaultClass, true);

    REQUIRE(net.getName() == "GND");
    REQUIRE(net.getSubnetNumber() == 1);
    REQUIRE(net.getNetNumber() == 100);
    REQUIRE(net.getNetClass() == &defaultClass);
    REQUIRE(net.containsPlane() == true);
  }

  SECTION("Net class assignment") {
    NetClass powerClass("Power", layers, clearance);
    Net net("VCC", 1, 101, &defaultClass);

    net.setNetClass(&powerClass);
    REQUIRE(net.getNetClass() == &powerClass);
  }

  SECTION("Net comparison") {
    Net net1("NET1", 1, 1, &defaultClass);
    Net net2("NET2", 1, 2, &defaultClass);
    Net net3("NET1", 1, 1, &defaultClass);

    REQUIRE(net1 == net3);  // Same net number
    REQUIRE_FALSE(net1 == net2);
    REQUIRE(net1 < net2);  // Alphabetic comparison
  }
}

TEST_CASE("Nets collection", "[rules][nets]") {
  LayerStructure layers = {Layer("F.Cu", true), Layer("B.Cu", true)};
  std::vector<std::string> clearanceNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, clearanceNames);
  NetClass defaultClass("Default", layers, clearance);

  SECTION("Add nets") {
    Nets nets;
    Net net1("GND", 1, 1, &defaultClass);
    Net net2("VCC", 1, 2, &defaultClass);

    nets.addNet(net1);
    nets.addNet(net2);

    REQUIRE(nets.count() == 2);
    REQUIRE(nets[0].getName() == "GND");
    REQUIRE(nets[1].getName() == "VCC");
  }

  SECTION("Find net by name") {
    Nets nets;
    Net net1("GND", 1, 1, &defaultClass);
    Net net2("VCC", 1, 2, &defaultClass);

    nets.addNet(net1);
    nets.addNet(net2);

    const Net* found = nets.getNet("VCC");
    REQUIRE(found != nullptr);
    REQUIRE(found->getNetNumber() == 2);

    found = nets.getNet("NotFound");
    REQUIRE(found == nullptr);
  }

  SECTION("Find net by number") {
    Nets nets;
    Net net1("GND", 1, 1, &defaultClass);
    Net net2("VCC", 1, 2, &defaultClass);

    nets.addNet(net1);
    nets.addNet(net2);

    const Net* found = nets.getNet(2);
    REQUIRE(found != nullptr);
    REQUIRE(found->getName() == "VCC");

    found = nets.getNet(999);
    REQUIRE(found == nullptr);
  }

  SECTION("Max net number") {
    Nets nets;
    nets.addNet(Net("N1", 1, 5, &defaultClass));
    nets.addNet(Net("N2", 1, 10, &defaultClass));
    nets.addNet(Net("N3", 1, 3, &defaultClass));

    REQUIRE(nets.maxNetNumber() == 10);
  }

  SECTION("Sort by name") {
    Nets nets;
    nets.addNet(Net("ZZZ", 1, 1, &defaultClass));
    nets.addNet(Net("AAA", 1, 2, &defaultClass));
    nets.addNet(Net("MMM", 1, 3, &defaultClass));

    nets.sortByName();

    REQUIRE(nets[0].getName() == "AAA");
    REQUIRE(nets[1].getName() == "MMM");
    REQUIRE(nets[2].getName() == "ZZZ");
  }
}
