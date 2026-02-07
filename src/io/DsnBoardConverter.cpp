#include "io/DsnBoardConverter.h"
#include "board/Trace.h"
#include "board/Via.h"
#include "board/Pin.h"
#include "core/Padstack.h"
#include <iostream>
#include <map>
#include <cmath>

namespace freerouting {

std::pair<std::unique_ptr<RoutingBoard>, ClearanceMatrix>
DsnBoardConverter::createRoutingBoard(const DsnDesign& dsn) {
  // Create layer structure
  LayerStructure layers = createLayerStructure(dsn);

  // Create clearance matrix
  ClearanceMatrix clearance = createClearanceMatrix(dsn, layers);

  // Create routing board
  auto board = std::make_unique<RoutingBoard>(layers, clearance);

  // Set nets
  // Note: Leaking memory here to match existing pattern - TODO: fix ownership model
  auto nets = new Nets();
  for (int netNum = 1; netNum <= static_cast<int>(dsn.network.nets.size()); ++netNum) {
    const auto& dsnNet = dsn.network.nets[netNum - 1];
    nets->addNet(Net(dsnNet.name, 1 /* subnetNumber */, netNum, nullptr /* netClass */, false));
  }
  board->setNets(nets);

  // Add components and pins
  addComponents(board.get(), dsn);

  // Add existing wiring (if any)
  addWiring(board.get(), dsn);

  return {std::move(board), clearance};
}

LayerStructure DsnBoardConverter::createLayerStructure(const DsnDesign& dsn) {
  std::vector<Layer> layerVec;

  for (const auto& dsnLayer : dsn.structure.layers) {
    bool isSignal = true;  // Default to signal
    if (dsnLayer.type == "power") {
      isSignal = false;
    }

    layerVec.emplace_back(dsnLayer.name, isSignal);
  }

  return LayerStructure(layerVec);
}

ClearanceMatrix DsnBoardConverter::createClearanceMatrix(
    const DsnDesign& dsn, const LayerStructure& layers) {

  // Create default clearance matrix with 2 classes (null, default)
  std::vector<std::string> classNames = {"null", "default"};
  ClearanceMatrix clearance(2, layers, classNames);

  // Default clearance value (0.2mm = 2000 internal units)
  int defaultClearance = 2000;

  // Parse DSN rules to find clearance values
  for (const auto& rule : dsn.structure.rules) {
    if (rule.type == "clearance" && !rule.values.empty()) {
      // Convert DSN units to internal units
      int ruleValue = dsn.toInternalUnits(static_cast<int>(rule.values[0]));
      defaultClearance = ruleValue;
      break;
    }
  }

  // Set default clearance for all layer pairs
  clearance.setDefaultValue(defaultClearance);

  return clearance;
}

void DsnBoardConverter::addWiring(RoutingBoard* board, const DsnDesign& dsn) {
  if (!board) return;

  // Add vias
  for (const auto& dsnVia : dsn.wiring.vias) {
    // Find net number
    int netNo = 0;
    const auto& nets = board->getNets();
    if (nets) {
      const Net* net = nets->getNet(dsnVia.netName);
      if (net) {
        netNo = net->getNetNumber();
      }
    }

    // Find padstack
    const DsnPadstack* padstackDef = dsn.findPadstack(dsnVia.padstackName);
    if (!padstackDef) continue;

    // Create padstack
    Padstack* padstack = new Padstack(
      dsnVia.padstackName,
      board->generateItemId(),
      0,  // from layer (will be set based on padstack def)
      board->getLayers().count() - 1,  // to layer
      true,  // attachAllowed
      false  // placedAbsolute
    );

    // Convert position to internal units
    IntPoint pos(
      dsn.toInternalUnits(dsnVia.position.x),
      dsn.toInternalUnits(dsnVia.position.y)
    );

    // Create via
    std::vector<int> netVec{netNo};
    int itemId = board->generateItemId();
    auto via = std::make_unique<Via>(
      pos, padstack, netVec, 0 /* clearance class */,
      itemId, FixedState::NotFixed, true /* attachAllowed */, board
    );

    board->addItem(std::move(via));
  }

  // Add traces
  for (const auto& dsnWire : dsn.wiring.wires) {
    // Find net number
    int netNo = 0;
    const auto& nets = board->getNets();
    if (nets) {
      const Net* net = nets->getNet(dsnWire.netName);
      if (net) {
        netNo = net->getNetNumber();
      }
    }

    // Add each path as trace segments
    for (const auto& path : dsnWire.paths) {
      // Find layer index
      int layerIndex = dsn.getLayerIndex(path.layer);
      if (layerIndex < 0) {
        continue;
      }

      // Convert width to internal units
      int halfWidth = dsn.toInternalUnits(path.width) / 2;

      // Create trace segments between consecutive points
      for (size_t i = 0; i + 1 < path.points.size(); ++i) {
        IntPoint p1(
          dsn.toInternalUnits(path.points[i].x),
          dsn.toInternalUnits(path.points[i].y)
        );
        IntPoint p2(
          dsn.toInternalUnits(path.points[i + 1].x),
          dsn.toInternalUnits(path.points[i + 1].y)
        );

        std::vector<int> netVec{netNo};
        int itemId = board->generateItemId();

        // Determine fixed state based on wire type
        FixedState fixedState = FixedState::NotFixed;
        if (dsnWire.type == "fix") {
          fixedState = FixedState::SystemFixed;
        } else if (dsnWire.type == "protect") {
          fixedState = FixedState::UserFixed;
        }

        auto trace = std::make_unique<Trace>(
          p1, p2, layerIndex, halfWidth, netVec,
          0 /* clearance class */, itemId, fixedState, board
        );

        board->addItem(std::move(trace));
      }
    }
  }
}

void DsnBoardConverter::addComponents(RoutingBoard* board, const DsnDesign& dsn) {
  if (!board) return;

  int componentNumber = 0;

  for (const auto& component : dsn.placement.components) {
    // Find the footprint image for this component
    const DsnImage* image = dsn.findImage(component.imageName);
    if (!image) {
      std::cerr << "Warning: Image '" << component.imageName << "' not found for component "
                << component.refdes << std::endl;
      continue;
    }

    componentNumber++;

    // Create a map of pin numbers to nets for this component
    std::map<std::string, int> pinToNet;

    for (const auto& net : dsn.network.nets) {
      for (const auto& pinRef : net.pins) {
        // Pin format is "refdes-pinNumber" (e.g., "U1-1")
        size_t dashPos = pinRef.find('-');
        if (dashPos != std::string::npos) {
          std::string refdes = pinRef.substr(0, dashPos);
          std::string pinNum = pinRef.substr(dashPos + 1);

          if (refdes == component.refdes) {
            const Net* netObj = board->getNets()->getNet(net.name);
            if (netObj) {
              pinToNet[pinNum] = netObj->getNetNumber();
            }
          }
        }
      }
    }


    // Calculate rotation matrix for component
    double rotRad = component.rotation * M_PI / 180.0;
    double cosRot = std::cos(rotRad);
    double sinRot = std::sin(rotRad);

    // Create pins for each pin in the image
    for (const auto& imagePin : image->pins) {
      // Calculate absolute pin position in DSN units
      // Rotate pin position and add to component position
      int rotatedX, rotatedY;
      if (component.onFront) {
        // Front side: normal rotation
        rotatedX = static_cast<int>(imagePin.position.x * cosRot - imagePin.position.y * sinRot);
        rotatedY = static_cast<int>(imagePin.position.x * sinRot + imagePin.position.y * cosRot);
      } else {
        // Back side: mirrored and rotated
        rotatedX = static_cast<int>(-imagePin.position.x * cosRot - imagePin.position.y * sinRot);
        rotatedY = static_cast<int>(-imagePin.position.x * sinRot + imagePin.position.y * cosRot);
      }

      // Convert to internal units
      IntPoint pinPos(
        dsn.toInternalUnits(component.position.x + rotatedX),
        dsn.toInternalUnits(component.position.y + rotatedY)
      );

      // Find which net this pin belongs to
      auto netIt = pinToNet.find(imagePin.pinNumber);
      std::vector<int> nets;
      if (netIt != pinToNet.end()) {
        nets.push_back(netIt->second);
      }

      // Create a simple padstack for the pin
      // For now, use a generic padstack - could enhance by parsing actual padstack shapes
      Padstack* padstack = new Padstack(
        imagePin.padstackName,
        board->generateItemId(),
        0,  // all layers
        board->getLayers().count() - 1,
        true,  // attach allowed
        false  // not absolute
      );

      // Create the pin
      int itemId = board->generateItemId();
      int pinNumber = 0;
      try {
        pinNumber = std::stoi(imagePin.pinNumber);
      } catch (...) {
        // Pin number might not be numeric, use hash or default
        pinNumber = static_cast<int>(std::hash<std::string>{}(imagePin.pinNumber) % 10000);
      }

      auto pin = std::make_unique<Pin>(
        pinPos, pinNumber, padstack, nets,
        0 /* clearance class */, itemId, componentNumber,
        FixedState::SystemFixed, board
      );

      board->addItem(std::move(pin));
    }
  }
}

} // namespace freerouting
