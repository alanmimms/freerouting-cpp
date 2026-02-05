#ifndef FREEROUTING_GEOMETRY_INTOCTAGON_H
#define FREEROUTING_GEOMETRY_INTOCTAGON_H

#include "Vector2.h"
#include "IntBox.h"
#include <algorithm>

namespace freerouting {

// Octagon with integer coordinates and 45-degree angle constraints
// Used for efficient PCB routing space representation with diagonal constraints
//
// The octagon is defined by 8 boundary parameters:
// - 4 axis-aligned: leftX, rightX, bottomY, topY
// - 4 diagonal (±45°): stored as x-axis intersections
//
// Diagonal line equations:
//   Lower-left to upper-right (+45°): y = x - lowerLeftDiagonalX
//   Upper-left to lower-right (-45°): y = -x + upperLeftDiagonalX
struct IntOctagon {
  // Axis-aligned boundaries
  int leftX;              // Left vertical border (min x)
  int bottomY;            // Bottom horizontal border (min y)
  int rightX;             // Right vertical border (max x)
  int topY;               // Top horizontal border (max y)

  // Diagonal boundaries (stored as x-axis intersections)
  int upperLeftDiagonalX;   // Upper-left (-45°) diagonal: y = -x + upperLeftDiagonalX
  int lowerRightDiagonalX;  // Lower-right (-45°) diagonal: y = -x + lowerRightDiagonalX
  int lowerLeftDiagonalX;   // Lower-left (+45°) diagonal: y = x - lowerLeftDiagonalX
  int upperRightDiagonalX;  // Upper-right (+45°) diagonal: y = x - upperRightDiagonalX

  // Empty octagon constant
  static constexpr IntOctagon empty() {
    return IntOctagon(kCritInt, kCritInt, -kCritInt, -kCritInt,
                      kCritInt, -kCritInt, kCritInt, -kCritInt);
  }

  // Default constructor creates empty octagon
  constexpr IntOctagon()
    : leftX(kCritInt), bottomY(kCritInt),
      rightX(-kCritInt), topY(-kCritInt),
      upperLeftDiagonalX(kCritInt), lowerRightDiagonalX(-kCritInt),
      lowerLeftDiagonalX(kCritInt), upperRightDiagonalX(-kCritInt) {}

  // Construct from 8 parameters
  // Order: leftX, bottomY, rightX, topY, upperLeftDiagonalX, lowerRightDiagonalX,
  //        lowerLeftDiagonalX, upperRightDiagonalX
  constexpr IntOctagon(int lx, int ly, int rx, int uy,
                       int ulx, int lrx, int llx, int urx)
    : leftX(lx), bottomY(ly), rightX(rx), topY(uy),
      upperLeftDiagonalX(ulx), lowerRightDiagonalX(lrx),
      lowerLeftDiagonalX(llx), upperRightDiagonalX(urx) {}

  // Construct octagon from a single point
  static constexpr IntOctagon fromPoint(IntPoint p) {
    int diagPlus = p.x - p.y;   // x - y for +45° lines
    int diagMinus = p.x + p.y;  // x + y for -45° lines
    return IntOctagon(p.x, p.y, p.x, p.y, diagMinus, diagMinus, diagPlus, diagPlus);
  }

  // Construct octagon from axis-aligned box (diagonals are unconstrained)
  static constexpr IntOctagon fromBox(const IntBox& box) {
    if (box.isEmpty()) return empty();

    // For unconstrained diagonals, use extreme values
    int diagMinPlus = box.ll.x - box.ur.y;  // Most negative for +45°
    int diagMaxPlus = box.ur.x - box.ll.y;  // Most positive for +45°
    int diagMinMinus = box.ll.x + box.ll.y;  // Most negative for -45°
    int diagMaxMinus = box.ur.x + box.ur.y;  // Most positive for -45°

    return IntOctagon(box.ll.x, box.ll.y, box.ur.x, box.ur.y,
                      diagMaxMinus, diagMinMinus, diagMinPlus, diagMaxPlus);
  }

  // Check if octagon is empty
  constexpr bool isEmpty() const {
    return leftX > rightX || bottomY > topY ||
           upperLeftDiagonalX < lowerRightDiagonalX ||
           lowerLeftDiagonalX > upperRightDiagonalX;
  }

  // Get dimension (0=point, 1=line, 2=area, -1=empty)
  constexpr int dimension() const {
    if (isEmpty()) return -1;

    bool hasWidth = (rightX > leftX);
    bool hasHeight = (topY > bottomY);
    bool hasDiagMinus = (lowerRightDiagonalX < upperLeftDiagonalX);
    bool hasDiagPlus = (lowerLeftDiagonalX < upperRightDiagonalX);

    if (hasWidth && hasHeight && hasDiagMinus && hasDiagPlus) {
      return 2; // Area
    } else if (rightX == leftX && topY == bottomY) {
      return 0; // Point
    } else {
      return 1; // Line
    }
  }

  // Get bounding box
  constexpr IntBox boundingBox() const {
    return IntBox(leftX, bottomY, rightX, topY);
  }

  // Get corner by index (0-7, counter-clockwise from bottom-left)
  constexpr IntPoint corner(int index) const {
    int i = index % 8;
    if (i < 0) i += 8;

    switch (i) {
      case 0: return IntPoint(lowerLeftDiagonalX - bottomY, bottomY);      // Bottom-left +45°
      case 1: return IntPoint(lowerRightDiagonalX + bottomY, bottomY);     // Bottom-right -45°
      case 2: return IntPoint(rightX, rightX - lowerRightDiagonalX);       // Right bottom -45°
      case 3: return IntPoint(rightX, upperRightDiagonalX - rightX);       // Right top +45°
      case 4: return IntPoint(upperRightDiagonalX - topY, topY);           // Top-right +45°
      case 5: return IntPoint(upperLeftDiagonalX + topY, topY);            // Top-left -45°
      case 6: return IntPoint(leftX, leftX - upperLeftDiagonalX);          // Left top -45°
      case 7: return IntPoint(leftX, lowerLeftDiagonalX - leftX);          // Left bottom +45°
      default: return IntPoint(0, 0);
    }
  }

  // Check if point is contained in octagon
  constexpr bool contains(IntPoint p) const {
    // Check axis-aligned bounds
    if (p.x < leftX || p.x > rightX || p.y < bottomY || p.y > topY) {
      return false;
    }

    // Check diagonal bounds
    int diagPlus = p.x - p.y;   // For +45° lines
    int diagMinus = p.x + p.y;  // For -45° lines

    return diagPlus >= lowerLeftDiagonalX && diagPlus <= upperRightDiagonalX &&
           diagMinus >= lowerRightDiagonalX && diagMinus <= upperLeftDiagonalX;
  }

  // Union with another octagon
  constexpr IntOctagon unionWith(const IntOctagon& other) const {
    if (isEmpty()) return other;
    if (other.isEmpty()) return *this;

    return IntOctagon(
      std::min(leftX, other.leftX),
      std::min(bottomY, other.bottomY),
      std::max(rightX, other.rightX),
      std::max(topY, other.topY),
      std::max(upperLeftDiagonalX, other.upperLeftDiagonalX),
      std::min(lowerRightDiagonalX, other.lowerRightDiagonalX),
      std::min(lowerLeftDiagonalX, other.lowerLeftDiagonalX),
      std::max(upperRightDiagonalX, other.upperRightDiagonalX)
    );
  }

  // Intersection with another octagon
  constexpr IntOctagon intersection(const IntOctagon& other) const {
    IntOctagon result(
      std::max(leftX, other.leftX),
      std::max(bottomY, other.bottomY),
      std::min(rightX, other.rightX),
      std::min(topY, other.topY),
      std::min(upperLeftDiagonalX, other.upperLeftDiagonalX),
      std::max(lowerRightDiagonalX, other.lowerRightDiagonalX),
      std::max(lowerLeftDiagonalX, other.lowerLeftDiagonalX),
      std::min(upperRightDiagonalX, other.upperRightDiagonalX)
    );
    return result.isEmpty() ? empty() : result;
  }

  // Expand octagon by offset in all directions
  constexpr IntOctagon expand(int offset) const {
    return IntOctagon(
      leftX - offset, bottomY - offset,
      rightX + offset, topY + offset,
      upperLeftDiagonalX + offset, lowerRightDiagonalX - offset,
      lowerLeftDiagonalX - offset, upperRightDiagonalX + offset
    );
  }

  // Translate octagon by vector
  constexpr IntOctagon translate(IntVector v) const {
    return IntOctagon(
      leftX + v.x, bottomY + v.y,
      rightX + v.x, topY + v.y,
      upperLeftDiagonalX + v.x + v.y,
      lowerRightDiagonalX + v.x + v.y,
      lowerLeftDiagonalX + v.x - v.y,
      upperRightDiagonalX + v.x - v.y
    );
  }

  // Equality
  constexpr bool operator==(const IntOctagon& other) const {
    if (isEmpty() && other.isEmpty()) return true;
    return leftX == other.leftX && bottomY == other.bottomY &&
           rightX == other.rightX && topY == other.topY &&
           upperLeftDiagonalX == other.upperLeftDiagonalX &&
           lowerRightDiagonalX == other.lowerRightDiagonalX &&
           lowerLeftDiagonalX == other.lowerLeftDiagonalX &&
           upperRightDiagonalX == other.upperRightDiagonalX;
  }

  constexpr bool operator!=(const IntOctagon& other) const {
    return !(*this == other);
  }
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_INTOCTAGON_H
