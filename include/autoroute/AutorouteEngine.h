#ifndef FREEROUTING_AUTOROUTE_AUTOROUTEENGINE_H
#define FREEROUTING_AUTOROUTE_AUTOROUTEENGINE_H

#include "autoroute/AutorouteControl.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/ExpansionRoomGenerator.h"
#include "board/RoutingBoard.h"
#include "datastructures/Stoppable.h"
#include "datastructures/TimeLimit.h"
#include <vector>
#include <memory>
#include <map>

namespace freerouting {

// Forward declarations
class Item;
class Via;
class ShapeSearchTree;

// Temporary autoroute data stored on the RoutingBoard
// Manages the routing process including expansion rooms and search state
class AutorouteEngine {
public:
  static constexpr int TRACE_WIDTH_TOLERANCE = 2;

  // Result states for autoroute attempts
  enum class AutorouteResult {
    Routed,        // Successfully routed
    AlreadyConnected, // Items are already connected
    NotRouted,     // Could not find a route
    Failed         // Error occurred
  };

  // The PCB board being routed
  RoutingBoard* board;

  // The current search tree used in autorouting
  ShapeSearchTree* autorouteSearchTree;

  // If true, maintain database after connection for performance
  bool maintainDatabase;

  // Creates a new AutorouteEngine
  AutorouteEngine(RoutingBoard* routingBoard, bool maintain = false)
    : board(routingBoard),
      autorouteSearchTree(nullptr),
      maintainDatabase(maintain),
      netNo(-1),
      stoppableThread(nullptr),
      timeLimit(nullptr),
      expansionRoomInstanceCount(0) {}

  // Initialize connection routing for a specific net
  void initConnection(int netNumber, Stoppable* stoppable, TimeLimit* timeLimit);

  // Auto-route a connection between start and destination items
  AutorouteResult autorouteConnection(
    const std::vector<Item*>& startSet,
    const std::vector<Item*>& destSet,
    const AutorouteControl& ctrl,
    std::vector<Item*>& rippedItems);

  // Returns the current net number being routed
  int getNetNo() const {
    return netNo;
  }

  // Check if stop has been requested
  bool isStopRequested() const;

  // Clear all temporary data
  void clear();

  // Reset all doors for next connection
  void resetAllDoors();

  // Add an incomplete expansion room
  IncompleteFreeSpaceExpansionRoom* addIncompleteExpansionRoom(
    const Shape* shape, int layer, const Shape* containedShape);

  // Get the first incomplete expansion room
  IncompleteFreeSpaceExpansionRoom* getFirstIncompleteExpansionRoom();

  // Remove an incomplete expansion room
  void removeIncompleteExpansionRoom(IncompleteFreeSpaceExpansionRoom* room);

  // Remove a complete expansion room
  void removeCompleteExpansionRoom(CompleteFreeSpaceExpansionRoom* room);

  // Complete an incomplete room (calculate final shape and add to complete list)
  CompleteFreeSpaceExpansionRoom* completeExpansionRoom(
    IncompleteFreeSpaceExpansionRoom* incompleteRoom);

  // Get expansion room count
  int getExpansionRoomCount() const {
    return expansionRoomInstanceCount;
  }

  // Get cached room generator for a specific net (creates on first access)
  ExpansionRoomGenerator* getRoomGenerator(int netNumber);

  // Clear cached room generators (call when board changes significantly)
  void clearRoomGenerators();

  // Complete neighbour rooms to ensure doors won't change during expansion
  void completeNeighbourRooms(ExpansionRoom* room);

private:
  int netNo; // Current net number
  Stoppable* stoppableThread;
  TimeLimit* timeLimit;

  // Lists of expansion rooms
  std::vector<std::unique_ptr<IncompleteFreeSpaceExpansionRoom>> incompleteExpansionRooms;
  std::vector<std::unique_ptr<CompleteFreeSpaceExpansionRoom>> completeExpansionRooms;

  int expansionRoomInstanceCount;

  // Ripup tracking: maps item ID -> number of times it's been ripped up
  std::map<int, int> ripupCounts;

  // Cached room generators per net (for performance)
  std::map<int, std::unique_ptr<ExpansionRoomGenerator>> roomGenerators;

  // Remove all doors from a room
  void removeAllDoors(ExpansionRoom* room);

  // Room completion helpers
  void calculateDoors(class ObstacleExpansionRoom* room);

  // Helper methods for routing
  AutorouteResult createDirectRoute(IntPoint start, IntPoint goal, int layer,
                                     const AutorouteControl& ctrl,
                                     int ripupCostLimit, std::vector<Item*>& rippedItems);
  AutorouteResult createTracesFromPath(const std::vector<IntPoint>& points, int layer,
                                        const AutorouteControl& ctrl,
                                        int ripupCostLimit, std::vector<Item*>& rippedItems);
  AutorouteResult routeWithVia(IntPoint start, IntPoint goal, int startLayer, int destLayer,
                                const AutorouteControl& ctrl,
                                int ripupCostLimit, std::vector<Item*>& rippedItems);
  AutorouteResult tryRouteWithViaAt(IntPoint viaLocation, IntPoint start, IntPoint goal,
                                     int startLayer, int destLayer,
                                     const AutorouteControl& ctrl,
                                     int ripupCostLimit, std::vector<Item*>& rippedItems);

  // Find existing via at a location (returns nullptr if not found)
  Via* findViaAtLocation(IntPoint location, int netNo) const;

  // Check if a trace would conflict with existing items
  // Returns list of conflicting items (obstacles)
  std::vector<Item*> findConflictingItems(IntPoint start, IntPoint end,
                                           int layer, int halfWidth, int netNo) const;

  // Calculate ripup cost for an item based on pass number
  int calculateRipupCost(Item* item, int passNumber) const;

  // Try to ripup conflicting items and add them to rippedItems
  // Returns true if ripup was successful (all conflicts can be ripped)
  bool ripupConflicts(const std::vector<Item*>& conflicts,
                      int ripupCostLimit,
                      std::vector<Item*>& rippedItems);
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_AUTOROUTEENGINE_H
