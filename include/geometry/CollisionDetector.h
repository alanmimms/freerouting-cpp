#ifndef FREEROUTING_GEOMETRY_COLLISIONDETECTOR_H
#define FREEROUTING_GEOMETRY_COLLISIONDETECTOR_H

#include "geometry/Shape.h"
#include "geometry/Circle.h"
#include "geometry/ConvexPolygon.h"
#include "geometry/IntBox.h"
#include "geometry/Vector2.h"
#include <cmath>

namespace freerouting {

// Static utility class for collision detection between shapes
// Provides various geometric intersection and containment tests
class CollisionDetector {
public:
  // Check if two circles intersect
  static bool circlesIntersect(const Circle& a, const Circle& b) {
    return a.intersects(b);
  }

  // Check if circle intersects box
  static bool circleBoxIntersect(const Circle& circle, const IntBox& box) {
    return circle.intersects(box);
  }

  // Check if two boxes intersect
  static bool boxesIntersect(const IntBox& a, const IntBox& b) {
    return a.intersects(b);
  }

  // Check if point is in circle
  static bool pointInCircle(IntPoint point, const Circle& circle) {
    return circle.contains(point);
  }

  // Check if point is in box
  static bool pointInBox(IntPoint point, const IntBox& box) {
    return box.contains(point);
  }

  // Check if point is in polygon
  static bool pointInPolygon(IntPoint point, const ConvexPolygon& polygon) {
    return polygon.contains(point);
  }

  // Calculate minimum distance between two boxes
  static double boxDistance(const IntBox& a, const IntBox& b) {
    // If boxes overlap, distance is 0
    if (a.intersects(b)) {
      return 0.0;
    }

    // Calculate horizontal and vertical gaps
    int horizontalGap = 0;
    if (a.ur.x < b.ll.x) {
      horizontalGap = b.ll.x - a.ur.x;
    } else if (b.ur.x < a.ll.x) {
      horizontalGap = a.ll.x - b.ur.x;
    }

    int verticalGap = 0;
    if (a.ur.y < b.ll.y) {
      verticalGap = b.ll.y - a.ur.y;
    } else if (b.ur.y < a.ll.y) {
      verticalGap = a.ll.y - b.ur.y;
    }

    // If only one gap is non-zero, that's the distance
    if (horizontalGap == 0) {
      return static_cast<double>(verticalGap);
    }
    if (verticalGap == 0) {
      return static_cast<double>(horizontalGap);
    }

    // Both gaps are non-zero, use Pythagorean theorem
    return std::sqrt(
      static_cast<double>(horizontalGap) * horizontalGap +
      static_cast<double>(verticalGap) * verticalGap
    );
  }

  // Calculate minimum distance between two circles
  static double circleDistance(const Circle& a, const Circle& b) {
    IntVector delta = b.getCenter() - a.getCenter();
    double centerDist = std::sqrt(static_cast<double>(delta.lengthSquared()));
    double surfaceDist = centerDist - a.getRadius() - b.getRadius();
    return std::max(0.0, surfaceDist);
  }

  // Calculate minimum distance between circle and box
  static double circleBoxDistance(const Circle& circle, const IntBox& box) {
    // Find nearest point on box to circle center
    IntPoint center = circle.getCenter();
    int nearestX = std::clamp(center.x, box.ll.x, box.ur.x);
    int nearestY = std::clamp(center.y, box.ll.y, box.ur.y);
    IntPoint nearest(nearestX, nearestY);

    // Distance from circle center to nearest point
    IntVector delta = nearest - center;
    double centerDist = std::sqrt(static_cast<double>(delta.lengthSquared()));
    double surfaceDist = centerDist - circle.getRadius();
    return std::max(0.0, surfaceDist);
  }

  // Check if line segment intersects box
  static bool segmentBoxIntersect(IntPoint a, IntPoint b, const IntBox& box) {
    // If either endpoint is inside box, they intersect
    if (box.contains(a) || box.contains(b)) {
      return true;
    }

    // Check if segment crosses any edge of box
    // This is a simplified test using bounding boxes
    int minX = std::min(a.x, b.x);
    int maxX = std::max(a.x, b.x);
    int minY = std::min(a.y, b.y);
    int maxY = std::max(a.y, b.y);

    IntBox segmentBox(minX, minY, maxX, maxY);
    return segmentBox.intersects(box);
  }

  // Check if line segment intersects circle
  static bool segmentCircleIntersect(IntPoint a, IntPoint b, const Circle& circle) {
    IntPoint center = circle.getCenter();

    // Vector from a to b
    IntVector ab = b - a;
    // Vector from a to center
    IntVector ac = center - a;

    i64 abLenSq = ab.lengthSquared();

    // Handle degenerate case (segment is a point)
    if (abLenSq == 0) {
      return circle.contains(a);
    }

    // Project center onto line segment
    i64 t = ac.x * ab.x + ac.y * ab.y;

    IntPoint closestPoint;
    if (t <= 0) {
      closestPoint = a;
    } else if (t >= abLenSq) {
      closestPoint = b;
    } else {
      double tNorm = static_cast<double>(t) / static_cast<double>(abLenSq);
      int projX = a.x + static_cast<int>(std::round(tNorm * ab.x));
      int projY = a.y + static_cast<int>(std::round(tNorm * ab.y));
      closestPoint = IntPoint(projX, projY);
    }

    return circle.contains(closestPoint);
  }

  // Calculate clearance between two boxes
  // Returns negative value if they overlap (amount of overlap)
  // Returns positive value if they don't (distance between them)
  static double boxClearance(const IntBox& a, const IntBox& b) {
    // Check for overlap in each dimension
    bool xOverlap = !(a.ur.x < b.ll.x || b.ur.x < a.ll.x);
    bool yOverlap = !(a.ur.y < b.ll.y || b.ur.y < a.ll.y);

    if (xOverlap && yOverlap) {
      // Boxes overlap - calculate overlap amount
      int xOverlapAmount = std::min(a.ur.x - b.ll.x, b.ur.x - a.ll.x);
      int yOverlapAmount = std::min(a.ur.y - b.ll.y, b.ur.y - a.ll.y);
      return -static_cast<double>(std::min(xOverlapAmount, yOverlapAmount));
    }

    // Boxes don't overlap - calculate distance
    return boxDistance(a, b);
  }

  // Expand box by offset amount (for collision testing with clearance)
  static IntBox expandBox(const IntBox& box, int offset) {
    return IntBox(
      box.ll.x - offset, box.ll.y - offset,
      box.ur.x + offset, box.ur.y + offset
    );
  }

  // Check if two boxes are within specified clearance distance
  static bool boxesWithinClearance(const IntBox& a, const IntBox& b, int clearance) {
    IntBox expandedA = expandBox(a, clearance);
    return expandedA.intersects(b);
  }

  // Check if two circles are within specified clearance distance
  static bool circlesWithinClearance(const Circle& a, const Circle& b, int clearance) {
    IntVector delta = b.getCenter() - a.getCenter();
    i64 centerDistSq = delta.lengthSquared();
    i64 requiredDist = static_cast<i64>(a.getRadius() + b.getRadius() + clearance);
    i64 requiredDistSq = requiredDist * requiredDist;
    return centerDistSq <= requiredDistSq;
  }

private:
  CollisionDetector() = delete;  // Static class, no instances
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_COLLISIONDETECTOR_H
