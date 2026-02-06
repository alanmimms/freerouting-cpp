#include "autoroute/AutorouteEngine.h"
#include "autoroute/ExpansionDoor.h"
#include "board/Item.h"
#include <algorithm>

namespace freerouting {

void AutorouteEngine::initConnection(int netNumber, Stoppable* stoppable, TimeLimit* limit) {
  if (maintainDatabase && netNumber != netNo) {
    // Invalidate net-dependent complete expansion rooms
    auto it = completeExpansionRooms.begin();
    while (it != completeExpansionRooms.end()) {
      if ((*it)->isNetDependent()) {
        // Remove from search tree if needed
        it = completeExpansionRooms.erase(it);
      } else {
        ++it;
      }
    }
  }

  netNo = netNumber;
  stoppableThread = stoppable;
  timeLimit = limit;
}

AutorouteEngine::AutorouteResult AutorouteEngine::autorouteConnection(
    const std::vector<Item*>& startSet,
    const std::vector<Item*>& destSet,
    const AutorouteControl& ctrl,
    std::vector<Item*>& rippedItems) {

  // Suppress unused parameter warnings for now
  (void)startSet;
  (void)destSet;
  (void)ctrl;
  (void)rippedItems;

  // Check if already connected
  // (This would need connectivity checking logic)

  // For now, return a placeholder
  // In a full implementation, this would:
  // 1. Create MazeSearchAlgo
  // 2. Call find_connection()
  // 3. Insert the found route into the board
  // 4. Handle ripup if needed

  return AutorouteResult::NotRouted;
}

bool AutorouteEngine::isStopRequested() const {
  if (timeLimit && timeLimit->isExceeded()) {
    return true;
  }
  if (stoppableThread) {
    return stoppableThread->isStopRequested();
  }
  return false;
}

void AutorouteEngine::clear() {
  // Remove complete rooms from search tree if needed
  completeExpansionRooms.clear();
  incompleteExpansionRooms.clear();
  expansionRoomInstanceCount = 0;

  // Clear autoroute data from board items
  if (board) {
    // This would call board->clearAllItemTemporaryAutorouteData()
    // when that method exists
  }
}

void AutorouteEngine::resetAllDoors() {
  for (auto& room : completeExpansionRooms) {
    room->resetDoors();
  }
  for (auto& room : incompleteExpansionRooms) {
    room->resetDoors();
  }
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::addIncompleteExpansionRoom(
    const Shape* shape, int layer, const Shape* containedShape) {

  auto room = std::make_unique<IncompleteFreeSpaceExpansionRoom>(
    shape, layer, containedShape);

  auto* roomPtr = room.get();
  incompleteExpansionRooms.push_back(std::move(room));

  return roomPtr;
}

IncompleteFreeSpaceExpansionRoom* AutorouteEngine::getFirstIncompleteExpansionRoom() {
  if (incompleteExpansionRooms.empty()) {
    return nullptr;
  }
  return incompleteExpansionRooms.front().get();
}

void AutorouteEngine::removeIncompleteExpansionRoom(IncompleteFreeSpaceExpansionRoom* room) {
  if (!room) return;

  removeAllDoors(room);

  auto it = std::find_if(incompleteExpansionRooms.begin(),
                         incompleteExpansionRooms.end(),
                         [room](const auto& ptr) { return ptr.get() == room; });

  if (it != incompleteExpansionRooms.end()) {
    incompleteExpansionRooms.erase(it);
  }
}

void AutorouteEngine::removeCompleteExpansionRoom(CompleteFreeSpaceExpansionRoom* room) {
  if (!room) return;

  // Create new incomplete expansion rooms for neighbors
  // (This would involve checking doors and creating new rooms)

  removeAllDoors(room);

  auto it = std::find_if(completeExpansionRooms.begin(),
                         completeExpansionRooms.end(),
                         [room](const auto& ptr) { return ptr.get() == room; });

  if (it != completeExpansionRooms.end()) {
    completeExpansionRooms.erase(it);
  }
}

CompleteFreeSpaceExpansionRoom* AutorouteEngine::completeExpansionRoom(
    IncompleteFreeSpaceExpansionRoom* incompleteRoom) {

  if (!incompleteRoom) return nullptr;

  // Create complete room with same shape and layer
  auto completeRoom = std::make_unique<CompleteFreeSpaceExpansionRoom>(
    incompleteRoom->getShape(),
    incompleteRoom->getLayer(),
    ++expansionRoomInstanceCount);

  // Transfer doors
  for (auto* door : incompleteRoom->getDoors()) {
    completeRoom->addDoor(door);
    // Update door references from incomplete to complete
    if (door->firstRoom == incompleteRoom) {
      door->firstRoom = completeRoom.get();
    }
    if (door->secondRoom == incompleteRoom) {
      door->secondRoom = completeRoom.get();
    }
  }

  auto* completePtr = completeRoom.get();
  completeExpansionRooms.push_back(std::move(completeRoom));

  // Remove the incomplete room
  removeIncompleteExpansionRoom(incompleteRoom);

  return completePtr;
}

void AutorouteEngine::removeAllDoors(ExpansionRoom* room) {
  if (!room) return;

  auto doors = room->getDoors();
  for (auto* door : doors) {
    // Remove door from the other room
    if (door->firstRoom == room && door->secondRoom) {
      door->secondRoom->removeDoor(door);
    } else if (door->secondRoom == room && door->firstRoom) {
      door->firstRoom->removeDoor(door);
    }
  }

  room->clearDoors();
}

} // namespace freerouting
