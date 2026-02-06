#include "autoroute/CompleteFreeSpaceExpansionRoom.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include <algorithm>

namespace freerouting {

bool CompleteFreeSpaceExpansionRoom::removeDoor(ExpandableObject* door) {
  // Try to remove as TargetItemExpansionDoor first
  auto* targetDoor = dynamic_cast<TargetItemExpansionDoor*>(door);
  if (targetDoor) {
    auto it = std::find(targetDoors.begin(), targetDoors.end(), targetDoor);
    if (it != targetDoors.end()) {
      targetDoors.erase(it);
      return true;
    }
    return false;
  }

  // Otherwise try base class removal
  return FreeSpaceExpansionRoom::removeDoor(door);
}

void CompleteFreeSpaceExpansionRoom::resetDoors() {
  // Reset regular doors
  FreeSpaceExpansionRoom::resetDoors();

  // Reset target doors
  for (auto* door : targetDoors) {
    if (door) {
      door->reset();
    }
  }
}

} // namespace freerouting
