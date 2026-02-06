#ifndef FREEROUTING_BOARD_COMPONENT_H
#define FREEROUTING_BOARD_COMPONENT_H

#include "geometry/Vector2.h"
#include <string>
#include <vector>

namespace freerouting {

// Forward declarations
class Item;

// Represents a PCB component consisting of pins and keepout areas
// Components have a location, rotation, and can be placed on front or back side
class Component {
public:
  // Create component
  Component(const std::string& componentName,
            IntPoint position,
            double rotationDegrees,
            bool onFront,
            int componentNo,
            bool positionFixed)
    : name(componentName),
      location(position),
      rotationInDegree(rotationDegrees),
      onFront(onFront),
      no(componentNo),
      positionFixed(positionFixed) {

    // Normalize rotation to [0, 360)
    while (rotationInDegree >= 360.0) {
      rotationInDegree -= 360.0;
    }
    while (rotationInDegree < 0.0) {
      rotationInDegree += 360.0;
    }
  }

  // Get component name
  const std::string& getName() const { return name; }

  // Get component number (unique ID)
  int getNo() const { return no; }

  // Get component location
  IntPoint getLocation() const { return location; }

  // Get rotation in degrees
  double getRotation() const { return rotationInDegree; }

  // Check if placed (has valid location)
  bool isPlaced() const { return location != IntPoint(); }

  // Check if on front side (vs back)
  bool isOnFront() const { return onFront; }

  // Check if position is fixed
  bool isPositionFixed() const { return positionFixed; }

  // Get pins belonging to this component
  // (Stub - requires Pin class)
  std::vector<Item*> getPins() const {
    // TODO: Implement when Pin class exists
    return {};
  }

private:
  std::string name;
  IntPoint location;
  double rotationInDegree;
  bool onFront;
  int no;              // Unique component number
  bool positionFixed;  // True if component cannot be moved
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_COMPONENT_H
