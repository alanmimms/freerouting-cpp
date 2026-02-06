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

  // Get items that need routing
  std::vector<Item*> itemsToRoute = getAutorouteItems();

  if (itemsToRoute.empty()) {
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
  lastPassStats.itemsQueued = static_cast<int>(itemsToRoute.size());
  lastPassStats.itemsRouted = 0;
  lastPassStats.itemsFailed = 0;
  lastPassStats.itemsSkipped = 0;
  lastPassStats.itemsRipped = 0;

  if (progressDisplay) {
    progressDisplay->startPass(passNumber);
    progressDisplay->message("Items to route: " + std::to_string(itemsToRoute.size()));
  }

  // Route each item
  for (Item* item : itemsToRoute) {
    if (stoppable && stoppable->isStopRequested()) {
      break;
    }

    // Get nets for this item
    std::vector<int> nets = item->getNets();
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
  Item* targetItem = nullptr;

  // Find first unrouted connection involving this item and net
  for (const auto& conn : connections) {
    if (conn.isRouted()) continue;
    if (conn.getNetNumber() != netNo) continue;

    if (conn.getFromItem() == item) {
      targetItem = conn.getToItem();
      foundConnection = true;
      break;
    }
    if (conn.getToItem() == item) {
      targetItem = conn.getFromItem();
      foundConnection = true;
      break;
    }
  }

  if (!foundConnection || !targetItem) {
    return AutorouteAttemptResult(AutorouteAttemptState::AlreadyConnected,
      "No unrouted connection found");
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
  control.ripupAllowed = (ripupPassNo > 1);  // Enable ripup after first pass
  control.ripupCosts = config.startRipupCosts * ripupPassNo;  // Increase cost with passes
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

std::vector<Item*> BatchAutorouter::getAutorouteItems() {
  std::vector<Item*> result;

  if (!board) {
    return result;
  }

  // Get incomplete connections from the board
  const auto& connections = board->getIncompleteConnections();

  // Collect unique items from unrouted connections
  std::set<Item*> uniqueItems;
  for (const auto& conn : connections) {
    if (!conn.isRouted()) {
      Item* fromItem = conn.getFromItem();
      Item* toItem = conn.getToItem();

      if (fromItem) uniqueItems.insert(fromItem);
      if (toItem) uniqueItems.insert(toItem);
    }
  }

  // Convert set to vector
  result.assign(uniqueItems.begin(), uniqueItems.end());

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
