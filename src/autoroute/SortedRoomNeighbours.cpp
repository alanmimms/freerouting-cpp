#include "autoroute/SortedRoomNeighbours.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/ObstacleExpansionRoom.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "autoroute/MazeSearchAlgo.h"
#include "board/Item.h"
#include "geometry/Shape.h"
#include "geometry/IntBox.h"

namespace freerouting {

// Forward declaration of helper function
static void createTargetDoorsForRoom(CompleteFreeSpaceExpansionRoom* room,
                                     AutorouteEngine* autorouteEngine);

CompleteExpansionRoom* SortedRoomNeighbours::calculate(ExpansionRoom* room,
                                                         AutorouteEngine* autorouteEngine) {
  // Main door calculation algorithm
  // Java: SortedRoomNeighbours.calculate() lines 52-86

  if (!room || !autorouteEngine) {
    return nullptr;
  }

  // If room is already complete, return it
  auto* completeRoom = dynamic_cast<CompleteExpansionRoom*>(room);
  if (completeRoom) {
    return completeRoom;
  }

  // If room is incomplete, complete it first
  auto* incompleteRoom = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(room);
  if (incompleteRoom) {
    completeRoom = autorouteEngine->completeExpansionRoom(incompleteRoom);
    if (!completeRoom) {
      return nullptr;
    }
  }

  // If we still don't have a complete room, something is wrong
  if (!completeRoom) {
    return nullptr;
  }

  // Phase 1: Minimal door generation
  // Create target doors to connectable items (pads/pins)
  // This is a simplified version that doesn't require spatial search tree

  // Try to downcast to CompleteFreeSpaceExpansionRoom (most common case)
  auto* freeSpaceRoom = dynamic_cast<CompleteFreeSpaceExpansionRoom*>(completeRoom);
  if (freeSpaceRoom) {
    createTargetDoorsForRoom(freeSpaceRoom, autorouteEngine);
  }

  // TODO Phase 2A: Add room-to-room doors
  // 1. Find overlapping/touching rooms in complete expansion room list
  // 2. Create ExpansionDoor objects to connect rooms
  // 3. Basic door validation (avoid duplicates)

  // TODO Phase 2B: Advanced features
  // 4. Sort neighbours counterclockwise
  // 5. Edge removal and room enlargement
  // 6. New incomplete room creation

  return completeRoom;
}

// Helper: Create target doors from room to connectable items
// Java: SortedRoomNeighbours.calculate_target_doors() lines 105-123
static void createTargetDoorsForRoom(CompleteFreeSpaceExpansionRoom* room,
                                     AutorouteEngine* autorouteEngine) {
  if (!room || !autorouteEngine || !autorouteEngine->board) {
    return;
  }

  int netNo = autorouteEngine->getNetNo();
  if (netNo < 0) {
    return; // No net being routed
  }

  const Shape* roomShape = room->getShape();
  if (!roomShape) {
    return;
  }

  int roomLayer = room->getLayer();
  IntBox roomBox = roomShape->getBoundingBox();

  // Iterate through all board items to find connectable items on this net
  // Java uses spatial search tree for efficiency, but for Phase 1 we iterate all items
  // TODO Phase 2: Replace with spatial search tree query for performance
  const auto& allItems = autorouteEngine->board->getItems();

  for (const auto& itemPtr : allItems) {
    Item* item = itemPtr.get();
    if (!item) {
      continue;
    }

    // Check if item is on this net
    if (!item->containsNet(netNo)) {
      continue;
    }

    // Check if item is on the same layer (or overlapping layers)
    // For simplicity, check if layer matches or item spans this layer
    int itemLayer = item->firstLayer();
    if (itemLayer != roomLayer) {
      // TODO: Handle multi-layer items (vias) properly
      continue;
    }

    // Quick bbox rejection test
    IntBox itemBox = item->getBoundingBox();
    if (!roomBox.intersects(itemBox)) {
      continue;
    }

    // For Phase 1, use bounding box intersection as proxy for shape intersection
    // TODO Phase 2: Use proper shape intersection (Shape::intersection) via search tree
    // Java gets shape via: item.get_tree_shape(search_tree, tree_entry_no)

    // Mark room as net dependent (overlaps with net-specific items)
    room->setNetDependent();

    // Create target door
    // Java passes tree_entry_no and uses search tree, we use 0 for now
    int treeEntryNo = 0;

    // For door shape, use room shape as approximation (Java intersects item & room shapes)
    // TODO Phase 2: Get actual item shape and compute intersection
    const Shape* doorShape = roomShape;

    TargetItemExpansionDoor* targetDoor = new TargetItemExpansionDoor(
      item, treeEntryNo, room, doorShape);

    room->addTargetDoor(targetDoor);
  }
}

} // namespace freerouting
