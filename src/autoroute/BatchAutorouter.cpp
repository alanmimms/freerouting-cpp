#include "autoroute/BatchAutorouter.h"
#include "autoroute/Connection.h"
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
    return false;
  }

  // Initialize pass statistics
  lastPassStats.passNumber = passNumber;
  lastPassStats.itemsQueued = static_cast<int>(itemsToRoute.size());
  lastPassStats.itemsRouted = 0;
  lastPassStats.itemsFailed = 0;
  lastPassStats.itemsSkipped = 0;
  lastPassStats.itemsRipped = 0;

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
          break;

        case AutorouteAttemptState::AlreadyConnected:
        case AutorouteAttemptState::NoUnconnectedNets:
        case AutorouteAttemptState::ConnectedToPlane:
        case AutorouteAttemptState::Skipped:
          ++lastPassStats.itemsSkipped;
          break;

        default:
          ++lastPassStats.itemsFailed;
          break;
      }
    }
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

  (void)rippedItems;
  (void)ripupPassNo;

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

  // Simple direct routing: create a trace between the two items
  // This is a placeholder - full implementation would use maze search

  // Get bounding box centers
  IntBox fromBox = item->getBoundingBox();
  IntBox toBox = targetItem->getBoundingBox();

  IntPoint fromCenter((fromBox.ll.x + fromBox.ur.x) / 2,
                      (fromBox.ll.y + fromBox.ur.y) / 2);
  IntPoint toCenter((toBox.ll.x + toBox.ur.x) / 2,
                    (toBox.ll.y + toBox.ur.y) / 2);

  // Use first common layer, or first layer of either item
  int layer = item->firstLayer();
  if (!targetItem->isOnLayer(layer)) {
    layer = targetItem->firstLayer();
  }

  // Create a simple trace (default 0.25mm = 2500 units at 10000 units/mm)
  int halfWidth = 1250;  // 0.125mm half-width = 0.25mm full width

  std::vector<int> nets{netNo};
  int itemId = board->generateItemId();

  auto trace = std::make_unique<Trace>(
    fromCenter, toCenter, layer, halfWidth,
    nets, 0 /* clearanceClass */, itemId,
    FixedState::NotFixed, board
  );

  board->addItem(std::move(trace));

  // Mark connection as routed
  board->markConnectionRouted(item, targetItem, netNo);

  return AutorouteAttemptResult(AutorouteAttemptState::Routed,
    "Simple direct route created");
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
