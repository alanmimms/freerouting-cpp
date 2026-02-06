#ifndef FREEROUTING_AUTOROUTE_COMPLETEFREESPACEEXPANSIONROOM_H
#define FREEROUTING_AUTOROUTE_COMPLETEFREESPACEEXPANSIONROOM_H

#include "autoroute/FreeSpaceExpansionRoom.h"
#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/TargetItemExpansionDoor.h"
#include "geometry/Shape.h"
#include <vector>

namespace freerouting {

// Forward declarations
class ShapeSearchTree;
class AutorouteEngine;

// An expansion room whose shape is completely calculated
// Can be stored in a shape tree for efficient spatial queries
class CompleteFreeSpaceExpansionRoom : public FreeSpaceExpansionRoom,
                                        public CompleteExpansionRoom {
public:
  // Creates a new CompleteFreeSpaceExpansionRoom
  CompleteFreeSpaceExpansionRoom(const Shape* roomShape, int layerNum, int idNum)
    : FreeSpaceExpansionRoom(roomShape, layerNum),
      idNo(idNum),
      roomIsNetDependent(false) {}

  ~CompleteFreeSpaceExpansionRoom() override = default;

  // Get target doors
  std::vector<TargetItemExpansionDoor*>& getTargetDoors() override {
    return targetDoors;
  }

  const std::vector<TargetItemExpansionDoor*>& getTargetDoors() const override {
    return targetDoors;
  }

  // Add a target door
  void addTargetDoor(TargetItemExpansionDoor* door) {
    targetDoors.push_back(door);
  }

  // Remove door (handles both regular doors and target doors)
  bool removeDoor(ExpandableObject* door) override;

  // Clear all doors including target doors
  void clearDoors() override {
    FreeSpaceExpansionRoom::clearDoors();
    targetDoors.clear();
  }

  // Reset all doors including target doors
  void resetDoors() override;

  // Mark room as net dependent
  // Called when the room overlaps with net dependent objects
  void setNetDependent() {
    roomIsNetDependent = true;
  }

  // Returns if the room overlaps with net dependent objects
  // Such rooms cannot be retained when the net number changes in autorouting
  bool isNetDependent() const {
    return roomIsNetDependent;
  }

  // Get the ID number
  int getIdNo() const {
    return idNo;
  }

  // Comparison for sorting (used when storing in trees)
  bool operator<(const CompleteFreeSpaceExpansionRoom& other) const {
    return idNo < other.idNo;
  }

private:
  int idNo;  // Identification number
  bool roomIsNetDependent;
  std::vector<TargetItemExpansionDoor*> targetDoors;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_COMPLETEFREESPACEEXPANSIONROOM_H
