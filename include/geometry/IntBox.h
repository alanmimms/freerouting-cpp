#ifndef FREEROUTING_GEOMETRY_INTBOX_H
#define FREEROUTING_GEOMETRY_INTBOX_H

#include "Vector2.h"
#include <algorithm>

namespace freerouting {

// Axis-aligned bounding box with integer coordinates
// Represents rectangle [ll.x, ur.x] × [ll.y, ur.y]
struct IntBox {
  IntPoint ll; // lower-left corner
  IntPoint ur; // upper-right corner

  // Empty box constant
  static const IntBox empty() {
    return IntBox(kCritInt, kCritInt, -kCritInt, -kCritInt);
  }

  // Default constructor creates empty box
  constexpr IntBox()
    : ll(kCritInt, kCritInt), ur(-kCritInt, -kCritInt) {}

  // Construct from corner points
  constexpr IntBox(IntPoint lowerLeft, IntPoint upperRight)
    : ll(lowerLeft), ur(upperRight) {}

  // Construct from coordinates
  constexpr IntBox(int llX, int llY, int urX, int urY)
    : ll(llX, llY), ur(urX, urY) {}

  // Construct box containing single point
  static constexpr IntBox fromPoint(IntPoint p) {
    return IntBox(p, p);
  }

  // Construct box containing two points
  static constexpr IntBox fromPoints(IntPoint a, IntPoint b) {
    return IntBox(
      std::min(a.x, b.x), std::min(a.y, b.y),
      std::max(a.x, b.x), std::max(a.y, b.y)
    );
  }

  // Check if box is empty
  constexpr bool isEmpty() const {
    return ll.x > ur.x || ll.y > ur.y;
  }

  // Width and height
  constexpr int width() const {
    return ur.x - ll.x;
  }

  constexpr int height() const {
    return ur.y - ll.y;
  }

  // Area
  constexpr double area() const {
    if (isEmpty()) return 0.0;
    return static_cast<double>(width()) * static_cast<double>(height());
  }

  // Perimeter
  constexpr double perimeter() const {
    if (isEmpty()) return 0.0;
    return 2.0 * (width() + height());
  }

  // Maximum dimension
  constexpr int maxDimension() const {
    return std::max(width(), height());
  }

  // Minimum dimension
  constexpr int minDimension() const {
    return std::min(width(), height());
  }

  // Center point (as floating point)
  constexpr FloatPoint center() const {
    return FloatPoint(
      (static_cast<double>(ll.x) + ur.x) / 2.0,
      (static_cast<double>(ll.y) + ur.y) / 2.0
    );
  }

  // Get corner by index (0=ll, 1=lr, 2=ur, 3=ul)
  constexpr IntPoint corner(int index) const {
    switch (index % 4) {
      case 0: return ll;                      // lower-left
      case 1: return IntPoint(ur.x, ll.y);    // lower-right
      case 2: return ur;                      // upper-right
      case 3: return IntPoint(ll.x, ur.y);    // upper-left
      default: return ll;
    }
  }

  // Check if point is contained in box
  constexpr bool contains(IntPoint p) const {
    return p.x >= ll.x && p.y >= ll.y && p.x <= ur.x && p.y <= ur.y;
  }

  // Check if box intersects another box
  constexpr bool intersects(const IntBox& other) const {
    return !(other.ll.x > ur.x || other.ur.x < ll.x ||
             other.ll.y > ur.y || other.ur.y < ll.y);
  }

  // Check if this box fully contains another box
  constexpr bool contains(const IntBox& other) const {
    if (other.isEmpty()) return true;
    if (isEmpty()) return false;
    return other.ll.x >= ll.x && other.ll.y >= ll.y &&
           other.ur.x <= ur.x && other.ur.y <= ur.y;
  }

  // Union with another box
  constexpr IntBox unionWith(const IntBox& other) const {
    if (isEmpty()) return other;
    if (other.isEmpty()) return *this;
    return IntBox(
      std::min(ll.x, other.ll.x), std::min(ll.y, other.ll.y),
      std::max(ur.x, other.ur.x), std::max(ur.y, other.ur.y)
    );
  }

  // Intersection with another box
  constexpr IntBox intersection(const IntBox& other) const {
    IntBox result(
      std::max(ll.x, other.ll.x), std::max(ll.y, other.ll.y),
      std::min(ur.x, other.ur.x), std::min(ur.y, other.ur.y)
    );
    return result.isEmpty() ? empty() : result;
  }

  // Expand box by offset in all directions
  constexpr IntBox expand(int offset) const {
    return IntBox(ll.x - offset, ll.y - offset, ur.x + offset, ur.y + offset);
  }

  // Expand box by different offsets in x and y
  constexpr IntBox expand(int offsetX, int offsetY) const {
    return IntBox(ll.x - offsetX, ll.y - offsetY, ur.x + offsetX, ur.y + offsetY);
  }

  // Translate box by vector
  constexpr IntBox translate(IntVector v) const {
    return IntBox(ll + v, ur + v);
  }

  // Equality
  constexpr bool operator==(const IntBox& other) const {
    // Empty boxes are all equal
    if (isEmpty() && other.isEmpty()) return true;
    return ll == other.ll && ur == other.ur;
  }

  constexpr bool operator!=(const IntBox& other) const {
    return !(*this == other);
  }
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_INTBOX_H
