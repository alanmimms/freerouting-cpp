#ifndef FREEROUTING_AUTOROUTE_EXPANSIONROOMGENERATOR_H
#define FREEROUTING_AUTOROUTE_EXPANSIONROOMGENERATOR_H

#include "geometry/Vector2.h"
#include "geometry/Shape.h"
#include "geometry/IntBoxShape.h"
#include "geometry/TileShape.h"
#include <vector>
#include <memory>
#include <map>

namespace freerouting {

class RoutingBoard;
class FreeSpaceExpansionRoom;
class IncompleteFreeSpaceExpansionRoom;
class ExpansionDoor;
class ShapeSearchTree;

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

  // ========== Phase 3B: Room Shape Completion ==========

  /**
   * Completes the shape of an incomplete room by restraining it around obstacles.
   *
   * Algorithm (from Java ShapeSearchTree.complete_shape() lines 549-631):
   * 1. Start with board bounding box (or initial shape if provided)
   * 2. Query search tree for all overlapping obstacles on the same layer
   * 3. For each obstacle that intersects the room:
   *    - Find the best edge of the obstacle to cut with
   *    - Restrain the room shape using that edge (half-plane intersection)
   * 4. Return the final constrained shape
   *
   * @param incompleteRoom The room being completed
   * @param containedShape Small shape that must remain inside the room (e.g., pad or trace end)
   * @param layer The layer this room is on
   * @param netNo The net number (obstacles on same net may not block)
   * @param searchTree Spatial index of board items and existing rooms
   * @return Completed room shape (nullptr if result is empty)
   */
  TileShape* completeRoomShape(
      IncompleteFreeSpaceExpansionRoom* incompleteRoom,
      const TileShape* containedShape,
      int layer,
      int netNo,
      ShapeSearchTree* searchTree);

  /**
   * Creates an initial room shape for a starting item.
   *
   * For now, creates a large box around the item.
   * In the future, this could be more sophisticated (e.g., octagon for 45° routing).
   *
   * @param layer The layer
   * @param itemBox Bounding box of the starting item
   * @param containedShape Shape that must be contained (typically same as itemBox)
   * @return Initial room shape
   */
  TileShape* createInitialRoomShape(
      int layer,
      const IntBox& itemBox,
      const TileShape* containedShape);

private:
  /**
   * Restrains a room shape to avoid an obstacle.
   *
   * Algorithm (from Java ShapeSearchTree.restrain_shape() lines 640-689):
   * 1. For each border edge of the obstacle:
   *    - Check if the edge intersects the room interior
   *    - Check if containedShape is on the correct side (away from obstacle)
   *    - Calculate distance from containedShape to the edge line
   * 2. Choose the edge with maximum distance to containedShape
   * 3. Cut the room with the opposite half-plane of that edge
   *
   * @param roomShape Current room shape
   * @param obstacleShape Shape of the obstacle
   * @param containedShape Shape that must remain in the room
   * @return Restrained room shape
   */
  TileShape* restrainShape(
      TileShape* roomShape,
      const TileShape* obstacleShape,
      const TileShape* containedShape);

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
