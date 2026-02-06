#ifndef FREEROUTING_AUTOROUTE_BATCHAUTOROUTER_H
#define FREEROUTING_AUTOROUTE_BATCHAUTOROUTER_H

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/AutorouteAttemptResult.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include "board/RoutingBoard.h"
#include <vector>
#include <memory>

namespace freerouting {

// Forward declarations
class Item;

// Handles the sequencing of the auto-router passes
// Routes multiple items across multiple passes with increasing ripup costs
class BatchAutorouter {
public:
  // Configuration for batch autorouting
  struct Config {
    int maxPasses = 100;              // Maximum number of routing passes
    int startRipupCosts = 100;        // Initial ripup cost factor
    bool removeUnconnectedVias = true;  // Remove unused vias after routing
    bool withPreferredDirections = true;  // Use preferred trace directions
    int tracePullTightAccuracy = 500;     // Trace optimization accuracy
  };

  // Statistics for a single routing pass
  struct PassStatistics {
    int passNumber = 0;
    int itemsQueued = 0;
    int itemsRouted = 0;
    int itemsFailed = 0;
    int itemsSkipped = 0;
    int itemsRipped = 0;
    int incompleteConnections = 0;
    double passDurationMs = 0.0;

    PassStatistics() = default;
  };

  // Create batch autorouter for board with default config
  explicit BatchAutorouter(RoutingBoard* board)
    : board(board),
      config(),
      stoppable(nullptr),
      currentPass(0) {}

  // Create batch autorouter for board with custom config
  BatchAutorouter(RoutingBoard* board, const Config& cfg)
    : board(board),
      config(cfg),
      stoppable(nullptr),
      currentPass(0) {}

  // Run the batch routing loop
  // Returns true if board is completely routed
  bool runBatchLoop(Stoppable* stoppable);

  // Route a single pass
  // Returns true if there are still unrouted items
  bool autoroutePass(int passNumber, Stoppable* stoppable);

  // Get current pass number
  int getCurrentPass() const { return currentPass; }

  // Get statistics for last completed pass
  const PassStatistics& getLastPassStats() const {
    return lastPassStats;
  }

private:
  // Route a single item on specific net
  AutorouteAttemptResult autorouteItem(
    Item* item,
    int netNo,
    std::vector<Item*>& rippedItems,
    int ripupPassNo);

  // Calculate airline distance between two item sets
  // Used for progress indication
  double calculateAirlineDistance(
    const std::vector<Item*>& fromSet,
    const std::vector<Item*>& toSet) const;

  // Get items that need routing
  std::vector<Item*> getAutorouteItems();

  // Clean up trace tails and unconnected vias
  void removeTails();

  RoutingBoard* board;
  Config config;
  Stoppable* stoppable;
  int currentPass;
  PassStatistics lastPassStats;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_BATCHAUTOROUTER_H
