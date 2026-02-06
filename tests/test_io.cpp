#include <catch2/catch_test_macros.hpp>
#include "io/SExprLexer.h"
#include "io/SExprParser.h"
#include "io/KiCadPcb.h"
#include "io/KiCadPcbReader.h"
#include "io/KiCadPcbWriter.h"

using namespace freerouting;

TEST_CASE("SExprLexer tokenization", "[io][sexpr][lexer]") {
  SECTION("Simple tokens") {
    SExprLexer lexer("(hello world)");

    auto t1 = lexer.nextToken();
    REQUIRE(t1.type == SExprTokenType::LeftParen);

    auto t2 = lexer.nextToken();
    REQUIRE(t2.type == SExprTokenType::Symbol);
    REQUIRE(t2.value == "hello");

    auto t3 = lexer.nextToken();
    REQUIRE(t3.type == SExprTokenType::Symbol);
    REQUIRE(t3.value == "world");

    auto t4 = lexer.nextToken();
    REQUIRE(t4.type == SExprTokenType::RightParen);

    auto t5 = lexer.nextToken();
    REQUIRE(t5.type == SExprTokenType::EndOfFile);
  }

  SECTION("String literals") {
    SExprLexer lexer("(name \"Hello World\")");

    lexer.nextToken();  // (
    auto t1 = lexer.nextToken();
    REQUIRE(t1.type == SExprTokenType::Symbol);
    REQUIRE(t1.value == "name");

    auto t2 = lexer.nextToken();
    REQUIRE(t2.type == SExprTokenType::String);
    REQUIRE(t2.value == "Hello World");
  }

  SECTION("Numbers") {
    SExprLexer lexer("(x 42 3.14 -10 +5.5)");

    lexer.nextToken();  // (
    lexer.nextToken();  // x

    auto t1 = lexer.nextToken();
    REQUIRE(t1.type == SExprTokenType::Number);
    REQUIRE(t1.value == "42");

    auto t2 = lexer.nextToken();
    REQUIRE(t2.type == SExprTokenType::Number);
    REQUIRE(t2.value == "3.14");

    auto t3 = lexer.nextToken();
    REQUIRE(t3.type == SExprTokenType::Number);
    REQUIRE(t3.value == "-10");

    auto t4 = lexer.nextToken();
    REQUIRE(t4.type == SExprTokenType::Number);
    REQUIRE(t4.value == "+5.5");
  }

  SECTION("Comments") {
    SExprLexer lexer("(hello # comment\nworld)");

    auto t1 = lexer.nextToken();
    REQUIRE(t1.type == SExprTokenType::LeftParen);

    auto t2 = lexer.nextToken();
    REQUIRE(t2.type == SExprTokenType::Symbol);
    REQUIRE(t2.value == "hello");

    auto t3 = lexer.nextToken();
    REQUIRE(t3.type == SExprTokenType::Symbol);
    REQUIRE(t3.value == "world");
  }

  SECTION("Escape sequences in strings") {
    SExprLexer lexer(R"("line1\nline2\ttab")");

    auto t1 = lexer.nextToken();
    REQUIRE(t1.type == SExprTokenType::String);
    REQUIRE(t1.value == "line1\nline2\ttab");
  }
}

TEST_CASE("SExprParser parsing", "[io][sexpr][parser]") {
  SECTION("Simple list") {
    SExprLexer lexer("(hello world)");
    SExprParser parser(lexer);

    auto node = parser.parse();
    REQUIRE(node != nullptr);
    REQUIRE(node->isList());
    REQUIRE(node->childCount() == 2);
    REQUIRE(node->getChild(0)->asString() == "hello");
    REQUIRE(node->getChild(1)->asString() == "world");
  }

  SECTION("Nested lists") {
    SExprLexer lexer("(outer (inner 42) test)");
    SExprParser parser(lexer);

    auto node = parser.parse();
    REQUIRE(node != nullptr);
    REQUIRE(node->isList());
    REQUIRE(node->childCount() == 3);

    REQUIRE(node->getChild(0)->asString() == "outer");
    REQUIRE(node->getChild(1)->isList());
    REQUIRE(node->getChild(1)->childCount() == 2);
    REQUIRE(node->getChild(2)->asString() == "test");
  }

  SECTION("Numbers in nodes") {
    SExprLexer lexer("(x 42 3.14)");
    SExprParser parser(lexer);

    auto node = parser.parse();
    REQUIRE(node != nullptr);
    REQUIRE(node->isList());

    REQUIRE(node->getChild(1)->asInt() == 42);
    REQUIRE(node->getChild(2)->asDouble() == 3.14);
  }

  SECTION("isListWithKeyword") {
    SExprLexer lexer("(kicad_pcb (version 20221018))");
    SExprParser parser(lexer);

    auto node = parser.parse();
    REQUIRE(node != nullptr);
    REQUIRE(node->isListWithKeyword("kicad_pcb"));
    REQUIRE_FALSE(node->isListWithKeyword("module"));
  }

  SECTION("Multiple top-level expressions") {
    SExprLexer lexer("(first) (second) (third)");
    SExprParser parser(lexer);

    auto nodes = parser.parseAll();
    REQUIRE(nodes.size() == 3);
    REQUIRE(nodes[0]->isListWithKeyword("first"));
    REQUIRE(nodes[1]->isListWithKeyword("second"));
    REQUIRE(nodes[2]->isListWithKeyword("third"));
  }
}

TEST_CASE("KiCadPcb structure", "[io][kicad]") {
  SECTION("Default construction") {
    KiCadPcb pcb;
    REQUIRE(pcb.version.version == 20221018);
    REQUIRE(pcb.general.thickness == 1.6);
    REQUIRE(pcb.nets.count() == 0);
  }

  SECTION("Layer access") {
    KiCadPcb pcb;
    pcb.layers = LayerStructure({
      Layer("F.Cu", true),
      Layer("B.Cu", true)
    });

    const Layer* fCu = pcb.getLayer(0);
    REQUIRE(fCu != nullptr);
    REQUIRE(fCu->name == "F.Cu");

    const Layer* bCu = pcb.getLayer(31);
    REQUIRE(bCu != nullptr);
    REQUIRE(bCu->name == "B.Cu");
  }
}

TEST_CASE("KiCadPcbReader parsing", "[io][kicad][reader]") {
  SECTION("Minimal PCB file") {
    std::string pcbContent = R"(
      (kicad_pcb (version 20221018) (generator pcbnew)
        (general (thickness 1.6))
        (paper "A4")
        (layers
          (0 "F.Cu" signal)
          (31 "B.Cu" signal))
      )
    )";

    auto pcb = KiCadPcbReader::readFromString(pcbContent);
    REQUIRE(pcb.has_value());
    REQUIRE(pcb->version.version == 20221018);
    REQUIRE(pcb->version.generator == "pcbnew");
    REQUIRE(pcb->general.thickness == 1.6);
    REQUIRE(pcb->paper == "A4");
    REQUIRE(pcb->layers.count() == 2);
  }

  SECTION("PCB with nets") {
    std::string pcbContent = R"(
      (kicad_pcb (version 20221018) (generator pcbnew)
        (general (thickness 1.6))
        (paper "A4")
        (layers
          (0 "F.Cu" signal))
        (net 0 "")
        (net 1 "GND")
        (net 2 "VCC")
      )
    )";

    auto pcb = KiCadPcbReader::readFromString(pcbContent);
    REQUIRE(pcb.has_value());
    REQUIRE(pcb->nets.count() == 3);

    const Net* gnd = pcb->getNet(1);
    REQUIRE(gnd != nullptr);
    REQUIRE(gnd->getName() == "GND");

    const Net* vcc = pcb->getNet("VCC");
    REQUIRE(vcc != nullptr);
    REQUIRE(vcc->getNetNumber() == 2);
  }

  SECTION("PCB with setup") {
    std::string pcbContent = R"(
      (kicad_pcb (version 20221018) (generator pcbnew)
        (general (thickness 1.2))
        (paper "A4")
        (layers
          (0 "F.Cu" signal))
        (setup
          (pad_to_mask_clearance 0.05))
      )
    )";

    auto pcb = KiCadPcbReader::readFromString(pcbContent);
    REQUIRE(pcb.has_value());
    REQUIRE(pcb->setup.padToMaskClearance == 0.05);
  }

  SECTION("PCB with net classes") {
    std::string pcbContent = R"(
      (kicad_pcb (version 20221018) (generator pcbnew)
        (general (thickness 1.6))
        (paper "A4")
        (layers (0 "F.Cu" signal))
        (net_class "Default" "This is the default net class.")
        (net_class "Power" "High current traces")
      )
    )";

    auto pcb = KiCadPcbReader::readFromString(pcbContent);
    REQUIRE(pcb.has_value());
    REQUIRE(pcb->netClassNames.size() == 2);
    REQUIRE(pcb->netClassNames[0] == "Default");
    REQUIRE(pcb->netClassNames[1] == "Power");
  }

  SECTION("Invalid PCB - missing layers") {
    std::string pcbContent = R"(
      (kicad_pcb (version 20221018)
        (paper "A4"))
    )";

    auto pcb = KiCadPcbReader::readFromString(pcbContent);
    // Should still parse but isValid() will be false
    REQUIRE(pcb.has_value());
    REQUIRE_FALSE(pcb->isValid());
  }
}

TEST_CASE("KiCadPcbWriter output", "[io][kicad][writer]") {
  SECTION("Write minimal PCB") {
    KiCadPcb pcb;
    pcb.version = KiCadVersion(20221018, "pcbnew");
    pcb.general.thickness = 1.6;
    pcb.paper = "A4";
    pcb.layers = LayerStructure({
      Layer("F.Cu", true),
      Layer("B.Cu", true)
    });

    std::string output = KiCadPcbWriter::writeToString(pcb);

    REQUIRE(output.find("(kicad_pcb") != std::string::npos);
    REQUIRE(output.find("(version 20221018)") != std::string::npos);
    REQUIRE(output.find("(generator pcbnew)") != std::string::npos);
    REQUIRE(output.find("(thickness 1.6") != std::string::npos);
    REQUIRE(output.find("(paper \"A4\")") != std::string::npos);
    REQUIRE(output.find("\"F.Cu\"") != std::string::npos);
    REQUIRE(output.find("\"B.Cu\"") != std::string::npos);
  }

  SECTION("Write PCB with nets") {
    KiCadPcb pcb;
    pcb.version = KiCadVersion(20221018, "pcbnew");
    pcb.paper = "A4";
    pcb.layers = LayerStructure({Layer("F.Cu", true)});

    Net net0("", 1, 0, nullptr);
    Net net1("GND", 1, 1, nullptr);
    Net net2("VCC", 1, 2, nullptr);

    pcb.nets.addNet(net0);
    pcb.nets.addNet(net1);
    pcb.nets.addNet(net2);

    std::string output = KiCadPcbWriter::writeToString(pcb);

    REQUIRE(output.find("(net 0 \"\")") != std::string::npos);
    REQUIRE(output.find("(net 1 \"GND\")") != std::string::npos);
    REQUIRE(output.find("(net 2 \"VCC\")") != std::string::npos);
  }
}

TEST_CASE("KiCad PCB round-trip", "[io][kicad][roundtrip]") {
  SECTION("Read and write produces similar output") {
    std::string original = R"(
(kicad_pcb (version 20221018) (generator pcbnew)
  (general
    (thickness 1.600000))
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal))
  (setup
    (pad_to_mask_clearance 0.000000))
  (net 0 "")
  (net 1 "GND")
  (net 2 "VCC")
)
    )";

    // Read
    auto pcb = KiCadPcbReader::readFromString(original);
    REQUIRE(pcb.has_value());

    // Write
    std::string output = KiCadPcbWriter::writeToString(*pcb);

    // Verify key elements are preserved
    REQUIRE(output.find("(kicad_pcb") != std::string::npos);
    REQUIRE(output.find("(version 20221018)") != std::string::npos);
    REQUIRE(output.find("(net 1 \"GND\")") != std::string::npos);
    REQUIRE(output.find("(net 2 \"VCC\")") != std::string::npos);
    REQUIRE(output.find("\"F.Cu\"") != std::string::npos);
    REQUIRE(output.find("\"B.Cu\"") != std::string::npos);
  }
}

TEST_CASE("KiCad PCB with board items - segments", "[io][kicad][boarditems]") {
  std::string pcbContent = R"(
(kicad_pcb (version 20221018) (generator pcbnew)
  (general (thickness 1.6))
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal))
  (net 0 "")
  (net 1 "GND")
  (segment (start 10.5 20.0) (end 30.5 20.0) (width 0.25) (layer "F.Cu") (net 1) (uuid "test-uuid-1"))
  (segment (start 30.5 20.0) (end 30.5 40.0) (width 0.25) (layer "F.Cu") (net 1) (uuid "test-uuid-2"))
)
  )";

  auto pcb = KiCadPcbReader::readFromString(pcbContent);
  REQUIRE(pcb.has_value());
  REQUIRE(pcb->segments.size() == 2);

  // Check first segment
  const auto& seg1 = pcb->segments[0];
  REQUIRE(seg1.startX == 10.5);
  REQUIRE(seg1.startY == 20.0);
  REQUIRE(seg1.endX == 30.5);
  REQUIRE(seg1.endY == 20.0);
  REQUIRE(seg1.width == 0.25);
  REQUIRE(seg1.layer == 0);  // F.Cu
  REQUIRE(seg1.netNumber == 1);
  REQUIRE(seg1.uuid == "test-uuid-1");

  // Check second segment
  const auto& seg2 = pcb->segments[1];
  REQUIRE(seg2.startX == 30.5);
  REQUIRE(seg2.endX == 30.5);
  REQUIRE(seg2.endY == 40.0);
}

TEST_CASE("KiCad PCB with board items - vias", "[io][kicad][boarditems]") {
  std::string pcbContent = R"(
(kicad_pcb (version 20221018) (generator pcbnew)
  (general (thickness 1.6))
  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal))
  (net 0 "")
  (net 1 "GND")
  (via (at 25.0 30.0) (size 0.8) (drill 0.4) (layers "F.Cu" "B.Cu") (net 1) (uuid "via-uuid-1"))
)
  )";

  auto pcb = KiCadPcbReader::readFromString(pcbContent);
  REQUIRE(pcb.has_value());
  REQUIRE(pcb->vias.size() == 1);

  const auto& via = pcb->vias[0];
  REQUIRE(via.x == 25.0);
  REQUIRE(via.y == 30.0);
  REQUIRE(via.size == 0.8);
  REQUIRE(via.drill == 0.4);
  REQUIRE(via.layersFrom == 0);  // F.Cu
  REQUIRE(via.layersTo == 1);    // B.Cu
  REQUIRE(via.netNumber == 1);
  REQUIRE(via.uuid == "via-uuid-1");
}

TEST_CASE("KiCad PCB with board items - footprints", "[io][kicad][boarditems]") {
  std::string pcbContent = R"(
(kicad_pcb (version 20221018) (generator pcbnew)
  (general (thickness 1.6))
  (paper "A4")
  (layers
    (0 "F.Cu" signal))
  (footprint "Resistor_SMD:R_0805" (at 50.0 50.0 90.0) (layer "F.Cu")
    (fp_text reference "R1" (at 0 0) (layer "F.SilkS"))
    (fp_text value "10K" (at 0 0) (layer "F.Fab"))
    (uuid "footprint-uuid-1"))
)
  )";

  auto pcb = KiCadPcbReader::readFromString(pcbContent);
  REQUIRE(pcb.has_value());
  REQUIRE(pcb->footprints.size() == 1);

  const auto& fp = pcb->footprints[0];
  REQUIRE(fp.x == 50.0);
  REQUIRE(fp.y == 50.0);
  REQUIRE(fp.rotation == 90.0);
  REQUIRE(fp.layer == 0);  // F.Cu
  REQUIRE(fp.reference == "R1");
  REQUIRE(fp.value == "10K");
  REQUIRE(fp.uuid == "footprint-uuid-1");
}

TEST_CASE("KiCad PCB board items round-trip", "[io][kicad][boarditems][roundtrip]") {
  // Create PCB with board items
  KiCadPcb pcb;
  pcb.version = KiCadVersion(20221018, "pcbnew");
  pcb.paper = "A4";
  pcb.layers = LayerStructure({Layer("F.Cu", true), Layer("B.Cu", true)});

  Net net1("GND", 1, 1, nullptr);
  pcb.nets.addNet(net1);

  // Add segment
  KiCadSegment seg;
  seg.startX = 10.0;
  seg.startY = 20.0;
  seg.endX = 30.0;
  seg.endY = 20.0;
  seg.width = 0.25;
  seg.layer = 0;
  seg.netNumber = 1;
  seg.uuid = "seg-1";
  pcb.segments.push_back(seg);

  // Add via
  KiCadVia via;
  via.x = 30.0;
  via.y = 20.0;
  via.size = 0.8;
  via.drill = 0.4;
  via.layersFrom = 0;
  via.layersTo = 1;
  via.netNumber = 1;
  via.uuid = "via-1";
  pcb.vias.push_back(via);

  // Write to string
  std::string output = KiCadPcbWriter::writeToString(pcb);

  // Verify board items are written
  REQUIRE(output.find("(segment") != std::string::npos);
  REQUIRE(output.find("(start 10") != std::string::npos);
  REQUIRE(output.find("(end 30") != std::string::npos);
  REQUIRE(output.find("(via") != std::string::npos);
  REQUIRE(output.find("(at 30") != std::string::npos);
  REQUIRE(output.find("(size 0.8") != std::string::npos);  // Relaxed to match formatted output
  REQUIRE(output.find("(drill 0.4") != std::string::npos);

  // Read back and verify
  auto pcbRead = KiCadPcbReader::readFromString(output);
  REQUIRE(pcbRead.has_value());
  REQUIRE(pcbRead->segments.size() == 1);
  REQUIRE(pcbRead->vias.size() == 1);
}
