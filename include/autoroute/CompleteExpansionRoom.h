#ifndef FREEROUTING_AUTOROUTE_COMPLETEEXPANSIONROOM_H
#define FREEROUTING_AUTOROUTE_COMPLETEEXPANSIONROOM_H

#include "autoroute/ExpansionRoom.h"
#include <vector>

namespace freerouting {

// Forward declarations
class TargetItemExpansionDoor;
class SearchTreeObject;

// Interface for expansion rooms whose shape is completely calculated
// so they can be stored in a shape tree
class CompleteExpansionRoom : public virtual ExpansionRoom {
public:
  virtual ~CompleteExpansionRoom() = default;

  // Returns the list of doors to target items of this room
  virtual std::vector<TargetItemExpansionDoor*>& getTargetDoors() = 0;
  virtual const std::vector<TargetItemExpansionDoor*>& getTargetDoors() const = 0;

  // Returns the search tree object of this complete expansion room
  // virtual SearchTreeObject* getObject() = 0;

protected:
  CompleteExpansionRoom() = default;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_COMPLETEEXPANSIONROOM_H
