#ifndef FREEROUTING_GEOMETRY_FLOATPOINT_H
#define FREEROUTING_GEOMETRY_FLOATPOINT_H

#include "geometry/Vector2.h"
#include <cmath>

namespace freerouting {

// Floating-point 2D point for precise geometric calculations
struct FloatPoint {
  double x;
  double y;

  FloatPoint() : x(0.0), y(0.0) {}
  FloatPoint(double xCoord, double yCoord) : x(xCoord), y(yCoord) {}

  // Create from IntPoint
  static FloatPoint fromInt(IntPoint p) {
    return FloatPoint(static_cast<double>(p.x), static_cast<double>(p.y));
  }

  // Convert to IntPoint (rounding)
  IntPoint round() const {
    return IntPoint(static_cast<int>(std::round(x)), static_cast<int>(std::round(y)));
  }

  // Calculate middle point between this and other
  FloatPoint middlePoint(const FloatPoint& other) const {
    return FloatPoint((x + other.x) / 2.0, (y + other.y) / 2.0);
  }

  // Calculate distance to another point
  double distance(const FloatPoint& other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    return std::sqrt(dx * dx + dy * dy);
  }

  // Calculate squared distance (faster, no sqrt)
  double distanceSquare(const FloatPoint& other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    return dx * dx + dy * dy;
  }

  // Weighted distance (for A* heuristic)
  double weightedDistance(const FloatPoint& other, double horizontalWeight, double verticalWeight) const {
    double dx = std::abs(x - other.x);
    double dy = std::abs(y - other.y);
    return dx * horizontalWeight + dy * verticalWeight;
  }

  // Equality comparison
  bool equals(const FloatPoint& other) const {
    constexpr double epsilon = 1e-6;
    return std::abs(x - other.x) < epsilon && std::abs(y - other.y) < epsilon;
  }

  bool operator==(const FloatPoint& other) const {
    return equals(other);
  }

  bool operator!=(const FloatPoint& other) const {
    return !equals(other);
  }
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_FLOATPOINT_H
