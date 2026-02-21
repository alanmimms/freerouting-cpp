#pragma once

#include "geometry/Vector2.h"

namespace freerouting {

/**
 * Represents an infinite directed line in 2D space.
 *
 * The line divides the plane into two half-planes:
 * - Left side: Points where ax + by + c > 0
 * - Right side: Points where ax + by + c < 0
 * - On line: Points where ax + by + c == 0
 *
 * The line equation is: ax + by + c = 0
 * The direction vector is (-b, a), perpendicular to the normal (a, b).
 *
 * Used for:
 * - Cutting room shapes with half-planes (room restraining)
 * - Finding which side of an obstacle edge a point/shape is on
 * - Detecting line-shape intersections
 */
class Line {
public:
  // Line equation coefficients: ax + by + c = 0
  double a;
  double b;
  double c;

  // ========== Constructors ==========

  /**
   * Creates a line with equation ax + by + c = 0.
   */
  Line(double a, double b, double c) : a(a), b(b), c(c) {}

  /**
   * Creates a line passing through two points.
   * The line direction is from p1 to p2.
   * Points to the left of the vector p1->p2 are on the left side.
   */
  static Line fromPoints(const IntPoint& p1, const IntPoint& p2);

  /**
   * Creates a line passing through two points.
   */
  static Line fromPoints(const FloatPoint& p1, const FloatPoint& p2);

  /**
   * Creates a horizontal line at y-coordinate.
   * Points above the line (y > yCoord) are on the left.
   */
  static Line horizontal(int yCoord);

  /**
   * Creates a vertical line at x-coordinate.
   * Points to the right of the line (x > xCoord) are on the left.
   */
  static Line vertical(int xCoord);

  // ========== Side Queries ==========

  /**
   * Returns the signed distance from the point to this line.
   * Positive = point is on the left side
   * Negative = point is on the right side
   * Zero = point is on the line
   */
  double sideOf(const IntPoint& point) const;
  double sideOf(const FloatPoint& point) const;

  /**
   * Returns true if the point is on the left side or on the line.
   */
  bool isOnLeftSide(const IntPoint& point) const {
    return sideOf(point) >= 0;
  }

  /**
   * Returns true if the point is on the right side or on the line.
   */
  bool isOnRightSide(const IntPoint& point) const {
    return sideOf(point) <= 0;
  }

  /**
   * Returns true if the point is exactly on the line (within epsilon).
   */
  bool isOnLine(const IntPoint& point, double epsilon = 0.001) const;

  // ========== Line Operations ==========

  /**
   * Returns a line with opposite direction (flips left/right sides).
   * Mathematically: -ax - by - c = 0, or equivalently ax + by + c = 0 with reversed normal.
   */
  Line opposite() const {
    return Line(-a, -b, -c);
  }

  /**
   * Returns the direction vector of this line.
   * Direction is perpendicular to normal (a, b), pointing "forward" along the line.
   */
  FloatPoint direction() const {
    // Direction perpendicular to normal (a, b) is (-b, a)
    return FloatPoint(-b, a);
  }

  /**
   * Returns the normal vector (a, b).
   * Points on the left side are in the direction of the normal.
   */
  FloatPoint normal() const {
    return FloatPoint(a, b);
  }

  /**
   * Normalizes the line equation so that a^2 + b^2 = 1.
   * This makes sideOf() return actual Euclidean distance.
   */
  void normalize();

  /**
   * Returns a normalized copy of this line.
   */
  Line normalized() const;

  // ========== Intersection ==========

  /**
   * Finds the intersection point of this line with another line.
   * Returns true if lines intersect at a unique point.
   * Returns false if lines are parallel or coincident.
   */
  bool intersect(const Line& other, FloatPoint& outPoint) const;

  // ========== Comparison ==========

  /**
   * Returns true if this line is equivalent to another (same equation).
   * Handles scaling: 2x + 2y + 2 = 0 equals x + y + 1 = 0.
   */
  bool equals(const Line& other, double epsilon = 0.001) const;

  // ========== String Representation ==========

  /**
   * Returns a string representation for debugging.
   */
  const char* toString() const;
};

} // namespace freerouting
