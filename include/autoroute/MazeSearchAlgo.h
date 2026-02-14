#ifndef FREEROUTING_AUTOROUTE_MAZESEARCHALGO_H
#define FREEROUTING_AUTOROUTE_MAZESEARCHALGO_H

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/DestinationDistance.h"
#include "autoroute/ExpandableObject.h"
#include "geometry/Vector2.h"
#include <vector>
#include <queue>
#include <set>
#include <memory>

namespace freerouting {

// Forward declarations
class Item;
class ExpansionRoom;

// Maze search algorithm for autorouting via expansion rooms
// Uses A* search to find paths through free space
class MazeSearchAlgo {
public:
  // Element in the maze expansion priority queue
  struct MazeListElement {
    ExpandableObject* door;          // The expansion door
    int sectionNo;                   // Section number in the door
    ExpansionRoom* nextRoom;         // Room to expand into
    int layer;                       // Layer number
    IntPoint location;               // Approximate location
    double sortingValue;             // Priority (g + h cost)
    double backtrackCost;            // Cost to reach this point (g)

    MazeListElement()
      : door(nullptr), sectionNo(0), nextRoom(nullptr),
        layer(0), location(), sortingValue(0.0), backtrackCost(0.0) {}

    // Comparison for priority queue (lower cost = higher priority)
    bool operator>(const MazeListElement& other) const {
      return sortingValue > other.sortingValue;
    }
  };

  // Result of the search
  struct Result {
    bool found;
    ExpandableObject* destinationDoor;
    int sectionNo;
    std::vector<IntPoint> pathPoints;
    std::vector<int> pathLayers;

    Result() : found(false), destinationDoor(nullptr), sectionNo(0) {}
  };

  // Create maze search algorithm instance
  static std::unique_ptr<MazeSearchAlgo> getInstance(
    const std::vector<Item*>& startItems,
    const std::vector<Item*>& destItems,
    AutorouteEngine* engine,
    const AutorouteControl& ctrl);

  // Find a connection between start and destination sets
  Result findConnection();

  // Constructor
  MazeSearchAlgo(AutorouteEngine* engine, const AutorouteControl& ctrl);

  // Check if expansion to a location is allowed by rule areas
  bool isExpansionAllowed(IntPoint point, int layer) const;

private:
  AutorouteEngine* autorouteEngine;
  const AutorouteControl& control;

  // Destination distance calculator for heuristics
  std::unique_ptr<DestinationDistance> destinationDistance;

  // Priority queue for expansion
  std::priority_queue<MazeListElement, std::vector<MazeListElement>,
                      std::greater<MazeListElement>> mazeExpansionList;

  // The destination door found by search
  ExpandableObject* destinationDoor;
  int sectionNoOfDestinationDoor;

  // Store start/dest items for SimpleGridRouter fallback
  std::vector<Item*> startItems;
  std::vector<Item*> destItems;

  // Initialize the search with start and destination items
  bool init(const std::vector<Item*>& startItems,
            const std::vector<Item*>& destItems);

  // Expand from a maze element
  void expand(const MazeListElement& element);

  // Calculate the cost to expand into a room/door
  double calculateExpansionCost(const MazeListElement& from,
                                 ExpandableObject* toDoor,
                                 int toSection,
                                 ExpansionRoom* toRoom) const;

  // Reconstruct the path by backtracking
  Result reconstructPath();

  // Check if an element is at a destination
  bool isDestination(ExpandableObject* door) const;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_MAZESEARCHALGO_H
