#include "autoroute/BatchAutorouter.h"
#include "autoroute/Connection.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/MSTRouter.h"
#include "board/Item.h"
#include "board/Trace.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <map>

namespace freerouting {

bool BatchAutorouter::runBatchLoop(Stoppable* stoppableThread) {
  this->stoppable = stoppableThread;
  currentPass = 0;

  bool continueRouting = true;

  while (continueRouting && currentPass < config.maxPasses) {
    if (stoppable && stoppable->isStopRequested()) {
      break;
    }

    ++currentPass;
    continueRouting = autoroutePass(currentPass, stoppable);
  }

  // Clean up at the end
  if (config.removeUnconnectedVias) {
    removeTails();
  }

  return !continueRouting;  // True if completely routed
}

bool BatchAutorouter::autoroutePass(int passNumber, Stoppable* stoppableThread) {
  this->stoppable = stoppableThread;

  // Get IDs of items that need routing
  std::vector<int> itemIdsToRoute = getAutorouteItemIds();

  if (itemIdsToRoute.empty()) {
    // Board is completely routed
    lastPassStats.passNumber = passNumber;
    lastPassStats.itemsQueued = 0;
    lastPassStats.itemsRouted = 0;
    lastPassStats.itemsFailed = 0;
    lastPassStats.itemsSkipped = 0;
    lastPassStats.incompleteConnections = 0;
    if (progressDisplay) {
      progressDisplay->message("All connections routed!", true);
    }
    return false;
  }

  // Initialize pass statistics
  lastPassStats.passNumber = passNumber;
  lastPassStats.itemsQueued = static_cast<int>(itemIdsToRoute.size());
  lastPassStats.itemsRouted = 0;
  lastPassStats.itemsFailed = 0;
  lastPassStats.itemsSkipped = 0;
  lastPassStats.itemsRipped = 0;

  if (progressDisplay) {
    progressDisplay->startPass(passNumber);
    progressDisplay->message("Items to route: " + std::to_string(itemIdsToRoute.size()));
  }

  // Group items by net for MST routing
  std::map<int, std::vector<Item*>> itemsByNet;

  for (int itemId : itemIdsToRoute) {
    Item* item = board->getItem(itemId);
    if (!item) continue;

    const std::vector<int>& nets = item->getNets();
    for (int netNo : nets) {
      itemsByNet[netNo].push_back(item);
    }
  }

  // Sort nets by complexity (simple nets first for better board utilization)
  // Priority: 2-pad nets → 3-5 pad nets → larger nets
  std::vector<std::pair<int, std::vector<Item*>>> sortedNets(
    itemsByNet.begin(), itemsByNet.end());

  std::sort(sortedNets.begin(), sortedNets.end(),
    [](const auto& a, const auto& b) {
      size_t sizeA = a.second.size();
      size_t sizeB = b.second.size();

      // Priority categories: 2-pad (highest), 3-5 pad (medium), >5 pad (lowest)
      auto getPriority = [](size_t size) {
        if (size <= 2) return 0;  // Highest priority
        if (size <= 5) return 1;  // Medium priority
        return 2;                 // Lowest priority
      };

      int priorityA = getPriority(sizeA);
      int priorityB = getPriority(sizeB);

      if (priorityA != priorityB) {
        return priorityA < priorityB;  // Lower number = higher priority
      }

      // Within same priority, route smaller nets first
      return sizeA < sizeB;
    });

  // Route each net in optimal order
  for (const auto& netPair : sortedNets) {
    if (stoppable && stoppable->isStopRequested()) {
      break;
    }

    int netNo = netPair.first;
    const std::vector<Item*>& netItems = netPair.second;

    // Skip empty nets
    if (netItems.empty()) {
      continue;
    }

    // For multi-pad nets (>2 items), use MST routing
    if (netItems.size() > 2) {
      AutorouteAttemptResult result = autorouteNetWithMST(
        netItems, netNo, passNumber);

      // Update statistics based on result
      switch (result.state) {
        case AutorouteAttemptState::Routed:
          lastPassStats.itemsRouted += static_cast<int>(netItems.size());
          if (progressDisplay) {
            progressDisplay->itemRouted("Net " + std::to_string(netNo) +
              " (MST: " + std::to_string(netItems.size()) + " pads)");
          }
          break;

        case AutorouteAttemptState::Skipped:
          lastPassStats.itemsSkipped += static_cast<int>(netItems.size());
          break;

        default:
          lastPassStats.itemsFailed += static_cast<int>(netItems.size());
          if (progressDisplay) {
            progressDisplay->itemFailed(result.details);
          }
          break;
      }
    } else {
      // For simple 2-pad nets, use existing point-to-point routing
      for (Item* item : netItems) {
        if (stoppable && stoppable->isStopRequested()) {
          break;
        }

        std::vector<Item*> rippedItems;
        AutorouteAttemptResult result = autorouteItem(
          item, netNo, rippedItems, passNumber);

        lastPassStats.itemsRipped += static_cast<int>(rippedItems.size());

        // Update statistics based on result
        switch (result.state) {
          case AutorouteAttemptState::Routed:
            ++lastPassStats.itemsRouted;
            if (progressDisplay) {
              progressDisplay->itemRouted("Net " + std::to_string(netNo));
            }
            break;

          case AutorouteAttemptState::AlreadyConnected:
          case AutorouteAttemptState::NoUnconnectedNets:
          case AutorouteAttemptState::ConnectedToPlane:
          case AutorouteAttemptState::Skipped:
            ++lastPassStats.itemsSkipped;
            break;

          default:
            ++lastPassStats.itemsFailed;
            if (progressDisplay) {
              progressDisplay->itemFailed(result.details);
            }
            break;
        }
      }
    }
  }

  // Show progress summary for this pass
  if (progressDisplay) {
    progressDisplay->message("Pass complete: " +
      std::to_string(lastPassStats.itemsRouted) + " routed, " +
      std::to_string(lastPassStats.itemsFailed) + " failed", true);
  }

  // Clean up after pass
  if (config.removeUnconnectedVias) {
    removeTails();
  }

  // Return true if we made progress (routed or failed items)
  return (lastPassStats.itemsRouted > 0 || lastPassStats.itemsFailed > 0);
}

AutorouteAttemptResult BatchAutorouter::autorouteItem(
    Item* item,
    int netNo,
    std::vector<Item*>& rippedItems,
    int ripupPassNo) {

  if (!board || !item) {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed, "No board or item");
  }

  // Check if this is a power/ground net - skip for now (needs special handling)
  const Nets* nets = board->getNets();
  if (nets) {
    const Net* net = nets->getNet(netNo);
    if (net) {
      std::string netName = net->getName();
      // Skip power/ground nets (common patterns)
      if (netName == "GND" || netName == "GNDA" || netName == "GNDD" ||
          netName == "VCC" || netName == "VDD" || netName == "VSS" ||
          netName.find("GND") != std::string::npos ||
          netName.find("+3V") != std::string::npos ||
          netName.find("+5V") != std::string::npos ||
          netName.find("+12V") != std::string::npos ||
          netName.find("-12V") != std::string::npos) {
        return AutorouteAttemptResult(AutorouteAttemptState::Skipped,
          "Power/ground net (needs copper pour)");
      }
    }
  }

  // Find incomplete connections for this item and net
  const auto& connections = board->getIncompleteConnections();

  bool foundConnection = false;
  int targetItemId = -1;

  // Find first unrouted connection involving this item and net
  // Compare by ID since vector reallocation may have invalidated connection pointers
  int itemId = item->getId();
  for (const auto& conn : connections) {
    if (conn.isRouted()) continue;
    if (conn.getNetNumber() != netNo) continue;

    Item* connFrom = conn.getFromItem();
    Item* connTo = conn.getToItem();

    if (connFrom && connFrom->getId() == itemId) {
      if (connTo) {
        targetItemId = connTo->getId();
        foundConnection = true;
        break;
      }
    }
    if (connTo && connTo->getId() == itemId) {
      if (connFrom) {
        targetItemId = connFrom->getId();
        foundConnection = true;
        break;
      }
    }
  }

  if (!foundConnection || targetItemId < 0) {
    return AutorouteAttemptResult(AutorouteAttemptState::AlreadyConnected,
      "No unrouted connection found");
  }

  // Look up target item by ID (safe even after vector reallocation)
  Item* targetItem = board->getItem(targetItemId);
  if (!targetItem) {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed,
      "Target item no longer exists");
  }

  // Use AutorouteEngine for pathfinding with obstacle avoidance
  AutorouteEngine engine(board);
  engine.initConnection(netNo, nullptr, nullptr);

  // Create AutorouteControl with routing parameters
  AutorouteControl control(board->getLayers().count());

  // Set trace widths for all layers (default 0.25mm = 1250 half-width)
  for (int i = 0; i < control.layerCount; ++i) {
    control.traceHalfWidth[i] = 1250;
  }

  // Configure ripup settings based on pass number
  // Enable ripup on all passes, but increase aggressiveness on later passes
  control.ripupAllowed = true;
  control.ripupCosts = config.startRipupCosts * std::max(1, ripupPassNo);  // Increase cost with passes
  control.ripupPassNo = ripupPassNo;

  // Adjust iteration limit based on net complexity (count connections on this net)
  int netConnectionCount = 0;
  for (const auto& conn : connections) {
    if (conn.getNetNumber() == netNo) {
      netConnectionCount++;
    }
  }

  // Dynamic iteration limit: base 50k + 5k per connection (sqrt scaling for large nets)
  // For GND with ~282 connections: 50k + 5k*sqrt(282) ≈ 50k + 84k = 134k
  // For simple 2-pad net: 50k + 5k*sqrt(1) = 55k
  control.maxIterations = 50000 + static_cast<int>(5000 * std::sqrt(std::max(1, netConnectionCount)));

  // Prepare start and destination sets
  std::vector<Item*> startSet{item};
  std::vector<Item*> destSet{targetItem};

  // Call the pathfinding algorithm
  auto result = engine.autorouteConnection(startSet, destSet, control, rippedItems);

  // Mark connection as routed if successful
  if (result == AutorouteEngine::AutorouteResult::Routed) {
    board->markConnectionRouted(item, targetItem, netNo);
    return AutorouteAttemptResult(AutorouteAttemptState::Routed,
      "Pathfinding route created");
  } else if (result == AutorouteEngine::AutorouteResult::AlreadyConnected) {
    return AutorouteAttemptResult(AutorouteAttemptState::AlreadyConnected,
      "Already connected");
  } else {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed,
      "Pathfinding failed");
  }
}

AutorouteAttemptResult BatchAutorouter::autorouteNetWithMST(
    const std::vector<Item*>& netItems,
    int netNo,
    int ripupPassNo) {

  if (!board || netItems.empty()) {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed, "No board or items");
  }

  // Check if this is a power/ground net - skip for now
  const Nets* nets = board->getNets();
  if (nets) {
    const Net* net = nets->getNet(netNo);
    if (net) {
      std::string netName = net->getName();
      // Skip power/ground nets (common patterns)
      if (netName == "GND" || netName == "GNDA" || netName == "GNDD" ||
          netName == "VCC" || netName == "VDD" || netName == "VSS" ||
          netName.find("GND") != std::string::npos ||
          netName.find("+3V") != std::string::npos ||
          netName.find("+5V") != std::string::npos ||
          netName.find("+12V") != std::string::npos ||
          netName.find("-12V") != std::string::npos) {
        return AutorouteAttemptResult(AutorouteAttemptState::Skipped,
          "Power/ground net (needs copper pour)");
      }
    }
  }

  // Build MST for this net
  std::vector<MSTRouter::MSTEdge> mstEdges = MSTRouter::buildMST(netItems);

  if (mstEdges.empty()) {
    return AutorouteAttemptResult(AutorouteAttemptState::AlreadyConnected,
      "No edges to route (single item or already connected)");
  }

  // Sort edges by cost (route shortest connections first)
  MSTRouter::sortEdgesByCost(mstEdges);

  // Route each MST edge
  int routedEdges = 0;
  int failedEdges = 0;

  for (const auto& edge : mstEdges) {
    if (stoppable && stoppable->isStopRequested()) {
      break;
    }

    // Use AutorouteEngine for pathfinding
    AutorouteEngine engine(board);
    engine.initConnection(netNo, nullptr, nullptr);

    // Create AutorouteControl with routing parameters
    AutorouteControl control(board->getLayers().count());

    // Set trace widths for all layers (default 0.25mm = 1250 half-width)
    for (int i = 0; i < control.layerCount; ++i) {
      control.traceHalfWidth[i] = 1250;
    }

    // Configure ripup settings
    control.ripupAllowed = true;
    control.ripupCosts = config.startRipupCosts * std::max(1, ripupPassNo);
    control.ripupPassNo = ripupPassNo;

    // Dynamic iteration limit based on net complexity
    const auto& connections = board->getIncompleteConnections();
    int netConnectionCount = 0;
    for (const auto& conn : connections) {
      if (conn.getNetNumber() == netNo) {
        netConnectionCount++;
      }
    }
    control.maxIterations = 50000 + static_cast<int>(5000 * std::sqrt(std::max(1, netConnectionCount)));

    // Route this edge
    std::vector<Item*> startSet{edge.from};
    std::vector<Item*> destSet{edge.to};
    std::vector<Item*> rippedItems;

    auto result = engine.autorouteConnection(startSet, destSet, control, rippedItems);

    if (result == AutorouteEngine::AutorouteResult::Routed ||
        result == AutorouteEngine::AutorouteResult::AlreadyConnected) {
      board->markConnectionRouted(edge.from, edge.to, netNo);
      routedEdges++;
    } else {
      failedEdges++;
    }
  }

  // Determine overall result
  if (failedEdges == 0) {
    return AutorouteAttemptResult(AutorouteAttemptState::Routed,
      "MST: " + std::to_string(routedEdges) + "/" + std::to_string(mstEdges.size()) + " edges");
  } else if (routedEdges > 0) {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed,
      "MST partial: " + std::to_string(routedEdges) + "/" + std::to_string(mstEdges.size()) + " edges");
  } else {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed,
      "MST failed: no edges routed");
  }
}

double BatchAutorouter::calculateAirlineDistance(
    const std::vector<Item*>& fromSet,
    const std::vector<Item*>& toSet) const {

  if (fromSet.empty() || toSet.empty()) {
    return 0.0;
  }

  // TODO: Full implementation requires Item::getPosition() or similar method
  // For now, return placeholder value
  (void)fromSet;
  (void)toSet;

  return 0.0;
}

std::vector<int> BatchAutorouter::getAutorouteItemIds() {
  std::vector<int> result;

  if (!board) {
    return result;
  }

  // Get incomplete connections from the board
  const auto& connections = board->getIncompleteConnections();

  // Collect unique item IDs from unrouted connections
  std::set<int> uniqueItemIds;
  for (const auto& conn : connections) {
    if (!conn.isRouted()) {
      Item* fromItem = conn.getFromItem();
      Item* toItem = conn.getToItem();

      if (fromItem) {
        uniqueItemIds.insert(fromItem->getId());
      }
      if (toItem) {
        uniqueItemIds.insert(toItem->getId());
      }
    }
  }

  // Convert set to vector
  result.assign(uniqueItemIds.begin(), uniqueItemIds.end());

  return result;
}

void BatchAutorouter::removeTails() {
  // TODO: Full implementation requires:
  // 1. Find all trace segments with only one connection (tails)
  // 2. Find all unconnected vias
  // 3. Remove these items from the board
  // 4. Optimize remaining connections

  // Stub for now
}

} // namespace freerouting
