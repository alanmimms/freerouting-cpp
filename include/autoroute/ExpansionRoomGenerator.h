#ifndef FREEROUTING_AUTOROUTE_EXPANSIONROOMGENERATOR_H
#define FREEROUTING_AUTOROUTE_EXPANSIONROOMGENERATOR_H

#include "geometry/Vector2.h"
#include "geometry/Shape.h"
#include <vector>
#include <memory>
#include <map>

namespace freerouting {

class RoutingBoard;
class FreeSpaceExpansionRoom;
class ExpansionDoor;

// Generates expansion rooms from board free space
// Simplified box-based approach for Phase 1
class ExpansionRoomGenerator {
public:
  ExpansionRoomGenerator(RoutingBoard* board, int netNo);
  ~ExpansionRoomGenerator();

  // Generate rooms for a specific layer
  // Uses simple grid-based subdivision of free space
  std::vector<FreeSpaceExpansionRoom*> generateRoomsForLayer(int layer);

  // Generate rooms only in a specific region (for performance)
  std::vector<FreeSpaceExpansionRoom*> generateRoomsInRegion(int layer, const IntBox& region);

  // Generate doors between overlapping rooms on same layer
  void generateDoorsForLayer(const std::vector<FreeSpaceExpansionRoom*>& rooms);

  // Generate doors (vias) between rooms on adjacent layers
  void generateViaDoors(
    const std::vector<FreeSpaceExpansionRoom*>& rooms1, int layer1,
    const std::vector<FreeSpaceExpansionRoom*>& rooms2, int layer2);

  // Find which room contains a point on a given layer
  FreeSpaceExpansionRoom* findRoomContaining(IntPoint point, int layer);

  // Cleanup - delete all allocated rooms and doors
  void cleanup();

private:
  RoutingBoard* board_;
  int netNo_;

  // All rooms generated (for cleanup)
  std::vector<FreeSpaceExpansionRoom*> allRooms_;

  // All doors generated (for cleanup)
  std::vector<ExpansionDoor*> allDoors_;

  // Cache of rooms per layer (for performance)
  std::map<int, std::vector<FreeSpaceExpansionRoom*>> roomsByLayer_;

  // Subdivide board into grid of potential rooms
  // Grid size adapts to average trace width
  std::vector<IntBox> generateGridCells(int layer, int gridSize);

  // Check if a box is free space (no obstacles for this net)
  bool isFreeSpace(const IntBox& box, int layer);

  // Internal method to actually generate rooms (called by public methods)
  std::vector<FreeSpaceExpansionRoom*> generateRoomsInternal(int layer);
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_EXPANSIONROOMGENERATOR_H
