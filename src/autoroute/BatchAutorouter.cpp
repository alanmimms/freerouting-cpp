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

  // Group item IDs by net for MST routing
  // IMPORTANT: Store item IDs, not pointers, because adding new items during routing
  // can cause the items vector to reallocate, invalidating all pointers!
  std::map<int, std::vector<int>> itemIdsByNet;

  for (int itemId : itemIdsToRoute) {
    Item* item = board->getItem(itemId);
    if (!item) continue;

    const std::vector<int>& nets = item->getNets();
    for (int netNo : nets) {
      itemIdsByNet[netNo].push_back(itemId);
    }
  }

  // Sort nets by complexity (simple nets first for better board utilization)
  // Priority: 2-pad nets → 3-5 pad nets → larger nets
  std::vector<std::pair<int, std::vector<int>>> sortedNets(
    itemIdsByNet.begin(), itemIdsByNet.end());

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
    const std::vector<int>& netItemIds = netPair.second;

    // Skip empty nets
    if (netItemIds.empty()) {
      continue;
    }

    // Convert item IDs to pointers RIGHT BEFORE USE
    // (Don't store pointers because vector reallocation can invalidate them!)
    std::vector<Item*> netItems;
    for (int itemId : netItemIds) {
      Item* item = board->getItem(itemId);
      if (item) {
        netItems.push_back(item);
      }
    }

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

  // Check if we should continue routing:
  // - Continue if we routed anything this pass (made progress)
  // - Stop if we didn't route anything (no progress)
  // Even if there are failures, keep trying if we're making progress
  bool madeProgress = (lastPassStats.itemsRouted > 0);

  // Also check if there are still incomplete connections
  bool hasIncompleteConnections = !board->getIncompleteConnections().empty();

  // Continue if we made progress AND still have incomplete connections
  return madeProgress && hasIncompleteConnections;
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
          netName == "VBUS" ||
          netName.find("GND") != std::string::npos ||
          netName.find("+3V") != std::string::npos ||
          netName.find("+5V") != std::string::npos ||
          netName.find("+12V") != std::string::npos ||
          netName.find("-12V") != std::string::npos ||
          netName.find("VA") != std::string::npos) {
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
  engine.initializeSearchTree();  // Populate search tree with board items (after initConnection clears it)

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

  // CRITICAL: Store item IDs immediately, not pointers!
  // Pointers will become invalid when we add traces/vias during routing
  std::vector<int> netItemIds;
  for (Item* item : netItems) {
    if (item) {
      netItemIds.push_back(item->getId());
    }
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
          netName == "VBUS" ||
          netName.find("GND") != std::string::npos ||
          netName.find("+3V") != std::string::npos ||
          netName.find("+5V") != std::string::npos ||
          netName.find("+12V") != std::string::npos ||
          netName.find("-12V") != std::string::npos ||
          netName.find("VA") != std::string::npos) {
        return AutorouteAttemptResult(AutorouteAttemptState::Skipped,
          "Power/ground net (needs copper pour)");
      }
    }
  }

  // Build MST for this net, but store INDICES not pointers
  // (pointers can become invalid when board's items vector reallocates!)
  struct MSTEdgeIndices {
    int fromIdx;
    int toIdx;
    double cost;
  };

  std::vector<MSTEdgeIndices> mstEdgeIndices;
  {
    // Build MST with pointers, but immediately convert to indices
    std::vector<MSTRouter::MSTEdge> mstEdges = MSTRouter::buildMST(netItems);

    if (mstEdges.empty()) {
      return AutorouteAttemptResult(AutorouteAttemptState::AlreadyConnected,
        "No edges to route (single item or already connected)");
    }

    // Convert edges to indices
    for (const auto& edge : mstEdges) {
      // Find indices in netItems vector
      int fromIdx = -1;
      int toIdx = -1;
      for (size_t i = 0; i < netItems.size(); ++i) {
        if (netItems[i] == edge.from) fromIdx = static_cast<int>(i);
        if (netItems[i] == edge.to) toIdx = static_cast<int>(i);
      }

      if (fromIdx >= 0 && toIdx >= 0) {
        mstEdgeIndices.push_back({fromIdx, toIdx, edge.cost});
      }
    }
  }

  if (mstEdgeIndices.empty()) {
    return AutorouteAttemptResult(AutorouteAttemptState::AlreadyConnected,
      "No edges to route (single item or already connected)");
  }

  // Sort edges by cost (route shortest connections first)
  std::sort(mstEdgeIndices.begin(), mstEdgeIndices.end(),
    [](const MSTEdgeIndices& a, const MSTEdgeIndices& b) {
      return a.cost < b.cost;
    });

  // Route each MST edge
  int routedEdges = 0;
  int failedEdges = 0;

  for (const auto& edgeIndices : mstEdgeIndices) {
    if (stoppable && stoppable->isStopRequested()) {
      break;
    }

    // Use AutorouteEngine for pathfinding
    AutorouteEngine engine(board);
    engine.initConnection(netNo, nullptr, nullptr);
    engine.initializeSearchTree();  // Populate search tree with board items (after initConnection clears it)

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

    // Get FRESH pointers right before routing (critical!)
    // Don't use stored pointers as they may have been invalidated by previous routing
    Item* fromItem = board->getItem(netItemIds[edgeIndices.fromIdx]);
    Item* toItem = board->getItem(netItemIds[edgeIndices.toIdx]);

    if (!fromItem || !toItem) {
      failedEdges++;
      continue;
    }

    // Route this edge
    std::vector<Item*> startSet{fromItem};
    std::vector<Item*> destSet{toItem};
    std::vector<Item*> rippedItems;

    auto result = engine.autorouteConnection(startSet, destSet, control, rippedItems);

    if (result == AutorouteEngine::AutorouteResult::Routed ||
        result == AutorouteEngine::AutorouteResult::AlreadyConnected) {
      board->markConnectionRouted(fromItem, toItem, netNo);
      routedEdges++;
    } else {
      failedEdges++;
    }
  }

  // Determine overall result
  if (failedEdges == 0) {
    return AutorouteAttemptResult(AutorouteAttemptState::Routed,
      "MST: " + std::to_string(routedEdges) + "/" + std::to_string(mstEdgeIndices.size()) + " edges");
  } else if (routedEdges > 0) {
    return AutorouteAttemptResult(AutorouteAttemptState::Failed,
      "MST partial: " + std::to_string(routedEdges) + "/" + std::to_string(mstEdgeIndices.size()) + " edges");
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
