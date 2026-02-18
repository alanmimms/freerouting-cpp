#ifndef FREEROUTING_AUTOROUTE_MAZESEARCHALGO_H
#define FREEROUTING_AUTOROUTE_MAZESEARCHALGO_H

#include "autoroute/AutorouteControl.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/DestinationDistance.h"
#include "autoroute/ExpandableObject.h"
#include "autoroute/MazeListElement.h"
#include "geometry/Vector2.h"
#include <vector>
#include <queue>
#include <set>
#include <memory>

namespace freerouting {

// Forward declarations
class Item;
class ExpansionRoom;
class ExpansionDoor;
class ObstacleExpansionRoom;
class Trace;

// Simplified FloatLine forward declaration (defined in .cpp)
struct FloatLine;

// Maze search algorithm for autorouting via expansion rooms
// Uses A* search to find paths through free space
class MazeSearchAlgo {
public:
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

  // Priority queue for expansion - using custom wrapper class
  class MazeExpansionList {
  public:
    void add(MazeListElement* element);
    MazeListElement* poll();
    bool empty() const { return queue.empty(); }
  private:
    std::vector<MazeListElement*> queue;
  };

  MazeExpansionList mazeExpansionList;

  // The destination door found by search
  ExpandableObject* destinationDoor;
  int sectionNoOfDestinationDoor;

  // Store start/dest items
  std::vector<Item*> startItems;
  std::vector<Item*> destItems;

  // Initialize the search with start and destination items
  bool init(const std::vector<Item*>& startItems,
            const std::vector<Item*>& destItems);

  // Main expansion loop - port of Java occupy_next_element()
  bool occupyNextElement();

  // Core expansion methods
  bool expandToRoomDoors(MazeListElement* pListElement);
  bool expandToDoor(ExpansionDoor* pToDoor, MazeListElement* pListElement,
                    int pAddCosts, bool pNextRoomIsThick, MazeSearchElement::Adjustment pAdjustment);
  bool expandToDoorSection(ExpandableObject* pDoor, int pSectionNo,
                            const FloatLine& pShapeEntry, MazeListElement* pFromElement,
                            int pAddCosts, MazeSearchElement::Adjustment pAdjustment);
  bool expandToTargetDoors(MazeListElement* pListElement, bool pNextRoomIsThick,
                            bool pCurrDoorIsSmall, const FloatPoint& pShapeEntryMiddle);
  void expandToOtherLayers(MazeListElement* pListElement);
  void expandToDrillsOfPage(MazeListElement* pFromElement);

  // Ripup and obstacle handling
  int checkRipup(MazeListElement* pListElement, Item* pObstacleItem, bool pDoorIsSmall);

  // Helper methods
  bool doorIsSmall(ExpansionDoor* pDoor, double pTraceWidth);
  bool roomShapeIsThick(ObstacleExpansionRoom* pObstacleRoom);
  double calcFanoutViaRipupCostFactor(Trace* pTrace);
  bool enterThroughSmallDoor(MazeListElement* pListElement, Item* pIgnoreItem);
  bool checkLeavingRippedItem(MazeListElement* pListElement);
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_MAZESEARCHALGO_H
