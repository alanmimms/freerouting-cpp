#include "autoroute/BatchAutorouter.h"
#include "autoroute/Connection.h"
#include "autoroute/AutorouteEngine.h"
#include "board/Item.h"
#include "board/Trace.h"
#include <algorithm>
#include <cmath>
#include <set>

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

  // Route each item (looked up by ID to avoid pointer invalidation)
  for (int itemId : itemIdsToRoute) {
    if (stoppable && stoppable->isStopRequested()) {
      break;
    }

    // Look up item by ID (safe even if items_ vector has been reallocated)
    Item* item = board->getItem(itemId);
    if (!item) {
      // Item may have been removed/merged during routing - skip it
      ++lastPassStats.itemsSkipped;
      continue;
    }

    // Get nets for this item
    const std::vector<int>& nets = item->getNets();
    for (int netNo : nets) {
      if (stoppable && stoppable->isStopRequested()) {
        break;
      }

      // Route this item on this net
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
