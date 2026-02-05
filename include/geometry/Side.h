#ifndef FREEROUTING_GEOMETRY_SIDE_H
#define FREEROUTING_GEOMETRY_SIDE_H

#include "core/Types.h"

namespace freerouting {

// Side enumeration for geometric predicates
// Used to determine the position of a point relative to a line
enum class Side {
  OnTheLeft,    // Point is on the left side of the line
  OnTheRight,   // Point is on the right side of the line
  Collinear     // Point is collinear with the line
};

// Create Side from a value (positive=left, negative=right, zero=collinear)
template<typename T>
constexpr Side sideFrom(T value) {
  if (value > T(0)) return Side::OnTheLeft;
  if (value < T(0)) return Side::OnTheRight;
  return Side::Collinear;
}

// Negate a side
constexpr Side negateSide(Side s) {
  switch (s) {
    case Side::OnTheLeft: return Side::OnTheRight;
    case Side::OnTheRight: return Side::OnTheLeft;
    case Side::Collinear: return Side::Collinear;
  }
  return Side::Collinear;
}

// Convert to string (for debugging)
constexpr const char* sideToString(Side s) {
  switch (s) {
    case Side::OnTheLeft: return "OnTheLeft";
    case Side::OnTheRight: return "OnTheRight";
    case Side::Collinear: return "Collinear";
  }
  return "Unknown";
}

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_SIDE_H
