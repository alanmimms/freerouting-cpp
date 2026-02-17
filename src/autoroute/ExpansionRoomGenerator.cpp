#include "autoroute/ExpansionRoomGenerator.h"
#include "autoroute/FreeSpaceExpansionRoom.h"
#include "autoroute/ExpansionDoor.h"
#include "board/RoutingBoard.h"
#include "geometry/Shape.h"
#include <algorithm>

namespace freerouting {

ExpansionRoomGenerator::ExpansionRoomGenerator(RoutingBoard* board, int netNo)
  : board_(board), netNo_(netNo) {
}

ExpansionRoomGenerator::~ExpansionRoomGenerator() {
  cleanup();
}

void ExpansionRoomGenerator::cleanup() {
  for (auto* room : allRooms_) {
    delete room;
  }
  allRooms_.clear();

  for (auto* door : allDoors_) {
    delete door;
  }
  allDoors_.clear();
}

std::vector<IntBox> ExpansionRoomGenerator::generateGridCells(int layer, int gridSize) {
  std::vector<IntBox> cells;

  // Get board bounding box from all items
  IntBox boardBounds(0, 0, 0, 0);
  bool first = true;

  const auto& items = board_->getItems();
  for (const auto& itemPtr : items) {
    if (!itemPtr) continue;

    IntBox bbox = itemPtr->getBoundingBox();
    if (first) {
      boardBounds = bbox;
      first = false;
    } else {
      boardBounds = boardBounds.unionWith(bbox);
    }
  }

  if (first) {
    // No items, use default bounds
    boardBounds = IntBox(-100000, -100000, 100000, 100000);
  }

  // Add margin
  const int margin = gridSize * 5;
  boardBounds.ll.x -= margin;
  boardBounds.ll.y -= margin;
  boardBounds.ur.x += margin;
  boardBounds.ur.y += margin;

  // Generate grid
  for (int y = boardBounds.ll.y; y < boardBounds.ur.y; y += gridSize) {
    for (int x = boardBounds.ll.x; x < boardBounds.ur.x; x += gridSize) {
      IntBox cell(x, y, x + gridSize, y + gridSize);
      cells.push_back(cell);
    }
  }

  return cells;
}

bool ExpansionRoomGenerator::isFreeSpace(const IntBox& box, int layer) {
  // Sample points in the box to check if free (2x2 = 4 points for performance)
  const int samples = 2;
  int dx = (box.ur.x - box.ll.x) / (samples + 1);
  int dy = (box.ur.y - box.ll.y) / (samples + 1);

  for (int sy = 1; sy <= samples; ++sy) {
    for (int sx = 1; sx <= samples; ++sx) {
      IntPoint p(box.ll.x + sx * dx, box.ll.y + sy * dy);

      // Check if prohibited for this net
      if (board_->isTraceProhibited(p, layer, netNo_)) {
        return false;
      }
    }
  }

  return true;
}

std::vector<FreeSpaceExpansionRoom*> ExpansionRoomGenerator::generateRoomsInternal(int layer) {
  std::vector<FreeSpaceExpansionRoom*> rooms;

  // Use coarse grid (10cm = 100000 internal units for performance)
  const int gridSize = 100000;

  auto cells = generateGridCells(layer, gridSize);

  for (const auto& cell : cells) {
    if (isFreeSpace(cell, layer)) {
      // Create room for this free space cell
      // Use nullptr for shape (simplified implementation)
      FreeSpaceExpansionRoom* room = new FreeSpaceExpansionRoom(nullptr, layer);

      // Store the box directly in the room
      room->setBounds(cell);

      rooms.push_back(room);
      allRooms_.push_back(room);
    }
  }

  return rooms;
}

std::vector<FreeSpaceExpansionRoom*> ExpansionRoomGenerator::generateRoomsForLayer(int layer) {
  // Check cache first
  auto it = roomsByLayer_.find(layer);
  if (it != roomsByLayer_.end()) {
    return it->second;  // Return cached rooms
  }

  // Generate and cache
  auto rooms = generateRoomsInternal(layer);
  roomsByLayer_[layer] = rooms;
  return rooms;
}

std::vector<FreeSpaceExpansionRoom*> ExpansionRoomGenerator::generateRoomsInRegion(int layer, const IntBox& region) {
  // For now, just return all rooms for the layer (lazy region filtering can be added later)
  return generateRoomsForLayer(layer);
}

void ExpansionRoomGenerator::generateDoorsForLayer(const std::vector<FreeSpaceExpansionRoom*>& rooms) {
  // Create doors between adjacent rooms (rooms that share an edge)
  for (size_t i = 0; i < rooms.size(); ++i) {
    for (size_t j = i + 1; j < rooms.size(); ++j) {
      IntBox box1 = rooms[i]->getBounds();
      IntBox box2 = rooms[j]->getBounds();

      // Check if they share an edge (are adjacent)
      bool adjacent = false;

      // Horizontal adjacency
      if (box1.ur.x == box2.ll.x || box2.ur.x == box1.ll.x) {
        int overlapY1 = std::max(box1.ll.y, box2.ll.y);
        int overlapY2 = std::min(box1.ur.y, box2.ur.y);
        if (overlapY2 > overlapY1) {
          adjacent = true;
        }
      }

      // Vertical adjacency
      if (box1.ur.y == box2.ll.y || box2.ur.y == box1.ll.y) {
        int overlapX1 = std::max(box1.ll.x, box2.ll.x);
        int overlapX2 = std::min(box1.ur.x, box2.ur.x);
        if (overlapX2 > overlapX1) {
          adjacent = true;
        }
      }

      if (adjacent && !rooms[i]->doorExists(rooms[j])) {
        // Create door between these rooms
        ExpansionDoor* door = new ExpansionDoor(rooms[i], rooms[j]);
        door->allocateSections(1);  // Allocate one section for simplified implementation
        rooms[i]->addDoor(door);
        rooms[j]->addDoor(door);
        allDoors_.push_back(door);
      }
    }
  }
}

void ExpansionRoomGenerator::generateViaDoors(
    const std::vector<FreeSpaceExpansionRoom*>& rooms1, int layer1,
    const std::vector<FreeSpaceExpansionRoom*>& rooms2, int layer2) {

  // Create via doors between overlapping rooms on different layers
  for (auto* room1 : rooms1) {
    for (auto* room2 : rooms2) {
      IntBox box1 = room1->getBounds();
      IntBox box2 = room2->getBounds();

      // Check if they overlap
      if (box1.intersects(box2)) {
        // Create via door
        ExpansionDoor* door = new ExpansionDoor(room1, room2);
        door->allocateSections(1);
        door->setIsVia(true);
        room1->addDoor(door);
        room2->addDoor(door);
        allDoors_.push_back(door);
      }
    }
  }
}

FreeSpaceExpansionRoom* ExpansionRoomGenerator::findRoomContaining(IntPoint point, int layer) {
  for (auto* room : allRooms_) {
    if (room->getLayer() == layer) {
      IntBox box = room->getBounds();
      if (box.contains(point)) {
        return room;
      }
    }
  }
  return nullptr;
}

} // namespace freerouting
