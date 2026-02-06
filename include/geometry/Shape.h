#ifndef FREEROUTING_GEOMETRY_SHAPE_H
#define FREEROUTING_GEOMETRY_SHAPE_H

#include "geometry/Vector2.h"
#include "geometry/IntBox.h"

namespace freerouting {

// Abstract base class for 2D geometric shapes
// Provides interface for collision detection, containment tests, and geometric queries
class Shape {
public:
  virtual ~Shape() = default;

  // Get bounding box containing this shape
  virtual IntBox getBoundingBox() const = 0;

  // Calculate area of shape
  virtual double area() const = 0;

  // Calculate perimeter/circumference of shape
  virtual double circumference() const = 0;

  // Check if point is inside shape (inclusive of boundary)
  virtual bool contains(IntPoint point) const = 0;

  // Check if point is strictly inside shape (exclusive of boundary)
  virtual bool containsInside(IntPoint point) const = 0;

  // Check if this shape intersects another shape
  virtual bool intersects(const Shape& other) const = 0;

  // Check if this shape intersects a box
  virtual bool intersects(const IntBox& box) const = 0;

  // Check if shape is empty (has no area)
  virtual bool isEmpty() const = 0;

  // Get dimension: -1 = empty, 0 = point, 1 = line, 2 = area
  virtual int dimension() const = 0;

  // Calculate distance from point to nearest point on shape
  virtual double distance(IntPoint point) const = 0;

  // Find nearest point on shape boundary to given point
  virtual IntPoint nearestPoint(IntPoint fromPoint) const = 0;

protected:
  Shape() = default;
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_SHAPE_H
