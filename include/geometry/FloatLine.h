#ifndef FREEROUTING_GEOMETRY_FLOATLINE_H
#define FREEROUTING_GEOMETRY_FLOATLINE_H

#include "geometry/FloatPoint.h"
#include <vector>
#include <cmath>

namespace freerouting {

// Floating-point line segment for precise geometric calculations
struct FloatLine {
  FloatPoint a;  // Start point
  FloatPoint b;  // End point

  FloatLine() : a(), b() {}
  FloatLine(FloatPoint start, FloatPoint end) : a(start), b(end) {}

  // Calculate the length of this line segment
  double length() const {
    return a.distance(b);
  }

  // Shrink the line segment by the given offset at both ends
  // Returns a shorter line segment, or the midpoint if too short
  FloatLine shrinkSegment(double offset) const {
    double len = length();
    if (len <= 2.0 * offset) {
      // Line too short, return midpoint
      FloatPoint mid = a.middlePoint(b);
      return FloatLine(mid, mid);
    }

    // Calculate direction vector
    double dx = b.x - a.x;
    double dy = b.y - a.y;

    // Normalize and scale by offset
    double scale = offset / len;
    double offsetX = dx * scale;
    double offsetY = dy * scale;

    // Shrink from both ends
    FloatPoint newA(a.x + offsetX, a.y + offsetY);
    FloatPoint newB(b.x - offsetX, b.y - offsetY);

    return FloatLine(newA, newB);
  }

  // Divide this line segment into sectionCount equal sections
  // Returns array of line segments representing each section
  std::vector<FloatLine> divideSegmentIntoSections(int sectionCount) const {
    std::vector<FloatLine> result;
    if (sectionCount <= 0) {
      return result;
    }

    if (sectionCount == 1) {
      result.push_back(*this);
      return result;
    }

    // Calculate delta per section
    double dx = (b.x - a.x) / sectionCount;
    double dy = (b.y - a.y) / sectionCount;

    for (int i = 0; i < sectionCount; i++) {
      FloatPoint start(a.x + i * dx, a.y + i * dy);
      FloatPoint end(a.x + (i + 1) * dx, a.y + (i + 1) * dy);
      result.push_back(FloatLine(start, end));
    }

    return result;
  }

  // Calculate the middle point of this line segment
  FloatPoint middlePoint() const {
    return a.middlePoint(b);
  }

  // Check if this line segment is degenerate (zero length)
  bool isDegenerate() const {
    constexpr double epsilon = 1e-6;
    return a.distanceSquare(b) < epsilon * epsilon;
  }
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_FLOATLINE_H
