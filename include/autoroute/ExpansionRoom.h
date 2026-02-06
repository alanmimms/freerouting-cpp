#ifndef FREEROUTING_AUTOROUTE_EXPANSIONROOM_H
#define FREEROUTING_AUTOROUTE_EXPANSIONROOM_H

#include "geometry/Shape.h"
#include <vector>

namespace freerouting {

// Forward declarations
class ExpansionDoor;
class ExpandableObject;

// Interface for expansion rooms used in maze search algorithm
// Expansion rooms represent routeable areas and obstacles on the PCB
class ExpansionRoom {
public:
  virtual ~ExpansionRoom() = default;

  // Adds door to the list of doors of this room
  virtual void addDoor(ExpansionDoor* door) = 0;

  // Returns the list of doors of this room to neighbour expansion rooms
  virtual std::vector<ExpansionDoor*>& getDoors() = 0;
  virtual const std::vector<ExpansionDoor*>& getDoors() const = 0;

  // Removes all doors from this room
  virtual void clearDoors() = 0;

  // Clears the autorouting info of all doors for routing the next connection
  virtual void resetDoors() = 0;

  // Checks if this room has already a door to other
  virtual bool doorExists(const ExpansionRoom* other) const = 0;

  // Removes door from this room
  // Returns false if room did not contain door
  virtual bool removeDoor(ExpandableObject* door) = 0;

  // Gets the shape of this room
  virtual const Shape* getShape() const = 0;

  // Returns the layer of this expansion room
  virtual int getLayer() const = 0;

protected:
  ExpansionRoom() = default;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_EXPANSIONROOM_H
