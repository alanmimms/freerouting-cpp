#include "autoroute/SortedRoomNeighbours.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/ObstacleExpansionRoom.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "autoroute/MazeSearchAlgo.h"
#include "autoroute/ShapeSearchTree.h"
#include "board/Item.h"
#include "geometry/Shape.h"
#include "geometry/IntBox.h"
#include <iostream>

namespace freerouting {

// Forward declarations of helper functions
static void createTargetDoorsForRoom(CompleteFreeSpaceExpansionRoom* room,
                                     AutorouteEngine* autorouteEngine);
static void createRoomToRoomDoors(CompleteFreeSpaceExpansionRoom* room,
                                   AutorouteEngine* autorouteEngine);

CompleteExpansionRoom* SortedRoomNeighbours::calculate(ExpansionRoom* room,
                                                         AutorouteEngine* autorouteEngine) {
  // Main door calculation algorithm
  // Java: SortedRoomNeighbours.calculate() lines 52-86

  if (!room || !autorouteEngine) {
    return nullptr;
  }

  // Get complete room (either already complete or complete it now)
  CompleteExpansionRoom* completeRoom = nullptr;

  // Try as already-complete room first
  completeRoom = dynamic_cast<CompleteExpansionRoom*>(room);

  // If not complete, try to complete it
  if (!completeRoom) {
    auto* incompleteRoom = dynamic_cast<IncompleteFreeSpaceExpansionRoom*>(room);
    if (incompleteRoom) {
      completeRoom = autorouteEngine->completeExpansionRoom(incompleteRoom);
      if (!completeRoom) {
        return nullptr;
      }
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

  // Phase 2A: Add room-to-room doors
  // Find rooms that touch this room and create doors between them
  if (freeSpaceRoom) {
    createRoomToRoomDoors(freeSpaceRoom, autorouteEngine);
  }

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
  int roomLayer = room->getLayer();

  // Get room bounding box
  // Phase 1: If room has no shape, use entire board as approximation
  // TODO Phase 2: Room should always have valid shape
  IntBox roomBox;
  if (roomShape) {
    roomBox = roomShape->getBoundingBox();
  } else {
    // Room has no shape - use maximum possible area
    // This is a Phase 1 workaround to allow door generation to proceed
    roomBox = IntBox(-kCritInt/2, -kCritInt/2, kCritInt/2, kCritInt/2);
  }

  // CRITICAL FIX: Only create target doors for items that the room ACTUALLY TOUCHES
  // The Java code creates target doors in calculate_target_doors() ONLY when:
  //   p_room.get_shape().intersects(curr_connection_shape)
  //
  // Previously, we created target doors to ALL items on the net, which caused
  // the maze search to find direct connections immediately (expansion list size = 1).
  //
  // Now we check if the room shape actually intersects the item's bounding box.
  // This ensures target doors are only created when a room physically reaches an item.

  const auto& allItems = autorouteEngine->board->getItems();
  int itemsChecked = 0;
  int itemsOnNet = 0;
  int itemsOnLayer = 0;
  int itemsIntersecting = 0;
  int doorsCreated = 0;

  for (const auto& itemPtr : allItems) {
    Item* item = itemPtr.get();
    if (!item) {
      continue;
    }
    itemsChecked++;

    // Check if item is on this net
    if (!item->containsNet(netNo)) {
      continue;
    }
    itemsOnNet++;

    // Check if item is on the same layer (or overlapping layers)
    int itemLayer = item->firstLayer();
    if (itemLayer != roomLayer) {
      // TODO: Handle multi-layer items (vias) properly
      continue;
    }
    itemsOnLayer++;

    // Only create target door if room shape INTERSECTS item
    // This ensures target doors are only created when a room physically reaches an item
    if (roomShape) {
      IntBox itemBox = item->getBoundingBox();
      if (!roomBox.intersects(itemBox)) {
        continue;  // Room doesn't touch this item
      }
    }
    itemsIntersecting++;

    // Room overlaps with net-specific items
    room->setNetDependent();

    // Create target door (only because room actually touches this item)
    int treeEntryNo = 0;
    const Shape* doorShape = roomShape;

    TargetItemExpansionDoor* targetDoor = new TargetItemExpansionDoor(
      item, treeEntryNo, room, doorShape);

    room->addTargetDoor(targetDoor);
    doorsCreated++;
  }

  (void)itemsChecked;
  (void)itemsOnNet;
  (void)itemsOnLayer;
  (void)itemsIntersecting;
  (void)doorsCreated;
}

// Helper: Create room-to-room doors for neighboring expansion rooms
// Java: SortedRoomNeighbours.calculate_neighbours() lines 125-231
static void createRoomToRoomDoors(CompleteFreeSpaceExpansionRoom* room,
                                   AutorouteEngine* autorouteEngine) {
  if (!room || !autorouteEngine) {
    return;
  }

  const Shape* roomShape = room->getShape();
  if (!roomShape) {
    return;  // Can't find neighbors without room shape
  }

  int roomLayer = room->getLayer();
  IntBox roomBox = roomShape->getBoundingBox();

  // Use spatial search tree to find overlapping rooms
  // Java: p_autoroute_search_tree.overlapping_tree_entries()
  if (!autorouteEngine->autorouteSearchTree) {
    std::cout << "createRoomToRoomDoors: No search tree!\n";
    return;  // No search tree available
  }

  (void)roomLayer;

  // Query for rooms that overlap with this room's bounding box on the same layer
  std::vector<TreeEntry> overlappingEntries;
  autorouteEngine->autorouteSearchTree->overlappingEntries(
    roomBox, roomLayer, overlappingEntries);

  // Create doors to overlapping objects (rooms and items)
  // Java: calculate_neighbours() lines 138-198
  for (const TreeEntry& entry : overlappingEntries) {
    SearchTreeObject* currObject = entry.object;
    if (!currObject || currObject == room) {
      continue;  // Skip self
    }

    // Try to cast to existing expansion room first
    CompleteExpansionRoom* otherRoom = dynamic_cast<CompleteExpansionRoom*>(currObject);

    // If not a room, try to cast to Item and get/create its obstacle room
    if (!otherRoom) {
      auto* item = dynamic_cast<Item*>(currObject);
      if (item && item->isRoutable()) {
        // Get or create obstacle expansion room for this item
        // Java: item_info.get_expansion_room(curr_entry.shape_index_in_object, p_autoroute_search_tree)
        // For now, create obstacle room on-the-fly
        otherRoom = new ObstacleExpansionRoom(item, entry.shapeIndex, autorouteEngine->autorouteSearchTree);
        // TODO: Cache in ItemAutorouteInfo to avoid recreating
      }
    }

    if (!otherRoom) {
      continue;  // Not a room or routable item
    }

    // Check if we already have a door to this room
    if (room->doorExists(otherRoom)) {
      continue;
    }

    // Get the intersection of the two room shapes
    const Shape* otherShape = otherRoom->getShape();
    if (!otherShape) {
      continue;
    }

    IntBox otherBox = otherShape->getBoundingBox();
    IntBox intersection = roomBox.intersection(otherBox);

    // Check dimension of intersection
    // 2D = area overlap, 1D = edge touch, 0D = corner touch, -1 = no intersection
    int intersectionDim = -1;
    if (!intersection.isEmpty()) {
      if (intersection.width() > 0 && intersection.height() > 0) {
        intersectionDim = 2;  // Area overlap
      } else if (intersection.width() == 0 || intersection.height() == 0) {
        intersectionDim = 1;  // Edge touch
      } else {
        intersectionDim = 0;  // Point touch
      }
    }

    // Only create doors for 1D intersections (shared edges)
    // Java: dimension == 1
    if (intersectionDim == 1) {
      // Create door connecting the two rooms
      // Java: new ExpansionDoor(completed_room, neighbour_room, 1)
      ExpansionDoor* door = new ExpansionDoor(room, otherRoom);

      // Add door to both rooms
      room->addDoor(door);
      otherRoom->addDoor(door);
    }
  }
}

} // namespace freerouting
