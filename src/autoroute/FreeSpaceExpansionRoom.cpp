#include "autoroute/FreeSpaceExpansionRoom.h"
#include "autoroute/ExpansionDoor.h"
#include "autoroute/ExpandableObject.h"
#include <algorithm>

namespace freerouting {

void FreeSpaceExpansionRoom::resetDoors() {
  for (auto* door : doors) {
    if (door) {
      door->reset();
    }
  }
}

bool FreeSpaceExpansionRoom::removeDoor(ExpandableObject* door) {
  auto* expDoor = dynamic_cast<ExpansionDoor*>(door);
  if (!expDoor) {
    return false;
  }

  auto it = std::find(doors.begin(), doors.end(), expDoor);
  if (it != doors.end()) {
    doors.erase(it);
    return true;
  }
  return false;
}

bool FreeSpaceExpansionRoom::doorExists(const ExpansionRoom* other) const {
  for (const auto* door : doors) {
    // Check if the door leads to "other" from this room
    if ((door->firstRoom == this && door->secondRoom == other) ||
        (door->secondRoom == this && door->firstRoom == other)) {
      return true;
    }
  }
  return false;
}

} // namespace freerouting
