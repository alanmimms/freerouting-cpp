#include "autoroute/ExpansionDoor.h"
#include "autoroute/CompleteExpansionRoom.h"

namespace freerouting {

ExpansionDoor::ExpansionDoor(ExpansionRoom* first, ExpansionRoom* second)
  : firstRoom(first), secondRoom(second), cachedShape(nullptr) {
  // Calculate dimension based on shape intersection
  // For now, assume dimension 1 (will be refined when we have proper shape intersection)
  dimension = 1;
}

const Shape* ExpansionDoor::getShape() const {
  // Return cached shape if available
  // TODO: Implement proper shape intersection when we have TileShape
  return cachedShape;
}

ExpansionRoom* ExpansionDoor::otherRoom(ExpansionRoom* room) {
  if (room == firstRoom) {
    return secondRoom;
  } else if (room == secondRoom) {
    return firstRoom;
  }
  return nullptr;
}

CompleteExpansionRoom* ExpansionDoor::otherRoom(CompleteExpansionRoom* room) {
  ExpansionRoom* result = nullptr;
  if (room == firstRoom) {
    result = secondRoom;
  } else if (room == secondRoom) {
    result = firstRoom;
  }

  // Check if result is a CompleteExpansionRoom
  return dynamic_cast<CompleteExpansionRoom*>(result);
}

void ExpansionDoor::reset() {
  for (auto& section : sectionArray) {
    section.reset();
  }
}

void ExpansionDoor::allocateSections(int sectionCount) {
  if (static_cast<int>(sectionArray.size()) == sectionCount) {
    return;  // Already allocated
  }
  sectionArray.resize(sectionCount);
}

} // namespace freerouting
