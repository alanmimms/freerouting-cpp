#ifndef FREEROUTING_GEOMETRY_CIRCLE_H
#define FREEROUTING_GEOMETRY_CIRCLE_H

#include "geometry/Shape.h"
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include <cmath>
#include <algorithm>

namespace freerouting {

// Circle shape with integer center and radius
// Used for circular pads, vias, and other round features
class Circle : public Shape {
public:
  // Construct circle with center and radius
  Circle(IntPoint centerPoint, int radiusValue)
    : center(centerPoint), radius(radiusValue) {}

  // Get center point
  IntPoint getCenter() const { return center; }

  // Get radius
  int getRadius() const { return radius; }

  // Get bounding box
  IntBox getBoundingBox() const override {
    return IntBox(center.x - radius, center.y - radius,
                  center.x + radius, center.y + radius);
  }

  // Calculate area
  double area() const override {
    constexpr double kPi = 3.14159265358979323846;
    return kPi * static_cast<double>(radius) * static_cast<double>(radius);
  }

  // Calculate circumference
  double circumference() const override {
    constexpr double kPi = 3.14159265358979323846;
    return 2.0 * kPi * static_cast<double>(radius);
  }

  // Check if point is inside circle (inclusive of boundary)
  bool contains(IntPoint point) const override {
    IntVector delta = point - center;
    i64 distSquared = delta.lengthSquared();
    i64 radiusSquared = static_cast<i64>(radius) * static_cast<i64>(radius);
    return distSquared <= radiusSquared;
  }

  // Check if point is strictly inside circle (exclusive of boundary)
  bool containsInside(IntPoint point) const override {
    IntVector delta = point - center;
    i64 distSquared = delta.lengthSquared();
    i64 radiusSquared = static_cast<i64>(radius) * static_cast<i64>(radius);
    return distSquared < radiusSquared;
  }

  // Check if this circle intersects another shape
  bool intersects(const Shape& other) const override {
    // Use bounding box as conservative test
    return getBoundingBox().intersects(other.getBoundingBox());
  }

  // Check if this circle intersects a box
  bool intersects(const IntBox& box) const override {
    // Find nearest point on box to circle center
    int nearestX = std::clamp(center.x, box.ll.x, box.ur.x);
    int nearestY = std::clamp(center.y, box.ll.y, box.ur.y);
    IntPoint nearest(nearestX, nearestY);

    // Check if nearest point is within radius
    IntVector delta = nearest - center;
    i64 distSquared = delta.lengthSquared();
    i64 radiusSquared = static_cast<i64>(radius) * static_cast<i64>(radius);
    return distSquared <= radiusSquared;
  }

  // Check if circle is empty
  bool isEmpty() const override {
    return radius <= 0;
  }

  // Get dimension
  int dimension() const override {
    if (radius <= 0) return -1;  // empty
    if (radius == 0) return 0;   // point (though radius <= 0 covers this)
    return 2;                     // area
  }

  // Calculate distance from point to circle
  double distance(IntPoint point) const override {
    IntVector delta = point - center;
    double centerDist = std::sqrt(static_cast<double>(delta.lengthSquared()));
    double edgeDist = centerDist - static_cast<double>(radius);
    return std::max(0.0, edgeDist);
  }

  // Find nearest point on circle to given point
  IntPoint nearestPoint(IntPoint fromPoint) const override {
    IntVector delta = fromPoint - center;

    // If point is at center, return arbitrary point on circle
    if (delta.x == 0 && delta.y == 0) {
      return IntPoint(center.x + radius, center.y);
    }

    // Normalize direction and scale to radius
    double length = std::sqrt(static_cast<double>(delta.lengthSquared()));
    double scale = static_cast<double>(radius) / length;

    int nearestX = center.x + static_cast<int>(std::round(delta.x * scale));
    int nearestY = center.y + static_cast<int>(std::round(delta.y * scale));

    return IntPoint(nearestX, nearestY);
  }

  // Check if two circles intersect
  bool intersects(const Circle& other) const {
    IntVector delta = other.center - center;
    i64 distSquared = delta.lengthSquared();
    i64 sumRadii = static_cast<i64>(radius) + static_cast<i64>(other.radius);
    i64 sumRadiiSquared = sumRadii * sumRadii;
    return distSquared <= sumRadiiSquared;
  }

  // Create offset circle (positive offset = larger, negative = smaller)
  Circle offset(int offsetAmount) const {
    return Circle(center, radius + offsetAmount);
  }

private:
  IntPoint center;
  int radius;
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_CIRCLE_H
