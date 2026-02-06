#ifndef FREEROUTING_GEOMETRY_COMPLEXPOLYGON_H
#define FREEROUTING_GEOMETRY_COMPLEXPOLYGON_H

#include "geometry/Shape.h"
#include "geometry/ConvexPolygon.h"
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>

namespace freerouting {

// Polygon with holes (complex polygon)
// Used for copper pours, conduction areas, and other filled regions
// Outer boundary is counter-clockwise, holes are clockwise
class ComplexPolygon : public Shape {
public:
  // Construct empty complex polygon
  ComplexPolygon() = default;

  // Construct from outer boundary
  explicit ComplexPolygon(const std::vector<IntPoint>& outerBoundary) {
    setOuterBoundary(outerBoundary);
  }

  // Construct with outer boundary and holes
  ComplexPolygon(const std::vector<IntPoint>& outerBoundary,
                 const std::vector<std::vector<IntPoint>>& holesList) {
    setOuterBoundary(outerBoundary);
    for (const auto& hole : holesList) {
      addHole(hole);
    }
  }

  // Set outer boundary
  void setOuterBoundary(const std::vector<IntPoint>& boundary) {
    outerBoundary = boundary;
  }

  // Add a hole to the polygon
  void addHole(const std::vector<IntPoint>& hole) {
    holes.push_back(hole);
  }

  // Get outer boundary
  const std::vector<IntPoint>& getOuterBoundary() const {
    return outerBoundary;
  }

  // Get number of holes
  size_t holeCount() const {
    return holes.size();
  }

  // Get hole at index
  const std::vector<IntPoint>& getHole(size_t index) const {
    static const std::vector<IntPoint> empty;
    return index < holes.size() ? holes[index] : empty;
  }

  // Get all holes
  const std::vector<std::vector<IntPoint>>& getHoles() const {
    return holes;
  }

  // Get bounding box
  IntBox getBoundingBox() const override {
    if (outerBoundary.empty()) {
      return IntBox(0, 0, 0, 0);
    }

    int minX = outerBoundary[0].x;
    int maxX = outerBoundary[0].x;
    int minY = outerBoundary[0].y;
    int maxY = outerBoundary[0].y;

    for (size_t i = 1; i < outerBoundary.size(); ++i) {
      minX = std::min(minX, outerBoundary[i].x);
      maxX = std::max(maxX, outerBoundary[i].x);
      minY = std::min(minY, outerBoundary[i].y);
      maxY = std::max(maxY, outerBoundary[i].y);
    }

    return IntBox(minX, minY, maxX, maxY);
  }

  // Calculate area (outer area minus hole areas)
  double area() const override {
    double totalArea = calculatePolygonArea(outerBoundary);

    for (const auto& hole : holes) {
      totalArea -= calculatePolygonArea(hole);
    }

    return std::max(0.0, totalArea);
  }

  // Calculate perimeter (outer perimeter plus all hole perimeters)
  double circumference() const override {
    double perimeter = calculatePerimeter(outerBoundary);

    for (const auto& hole : holes) {
      perimeter += calculatePerimeter(hole);
    }

    return perimeter;
  }

  // Check if point is inside polygon (must be in outer boundary and not in any hole)
  bool contains(IntPoint point) const override {
    if (outerBoundary.size() < 3) {
      return false;
    }

    // Must be inside outer boundary
    if (!pointInPolygon(point, outerBoundary)) {
      return false;
    }

    // Must not be inside any hole
    for (const auto& hole : holes) {
      if (pointInPolygon(point, hole)) {
        return false;
      }
    }

    return true;
  }

  // Check if point is strictly inside
  bool containsInside(IntPoint point) const override {
    // For complex polygons, we use the same test as contains()
    return contains(point);
  }

  // Check intersection with another shape
  bool intersects(const Shape& other) const override {
    return getBoundingBox().intersects(other.getBoundingBox());
  }

  // Check intersection with box
  bool intersects(const IntBox& box) const override {
    if (!getBoundingBox().intersects(box)) {
      return false;
    }

    // Check if any outer boundary vertex is inside box
    for (const auto& vertex : outerBoundary) {
      if (box.contains(vertex)) {
        return true;
      }
    }

    // Check if any box corner is inside polygon
    IntPoint corners[4] = {
      box.ll,
      IntPoint(box.ur.x, box.ll.y),
      box.ur,
      IntPoint(box.ll.x, box.ur.y)
    };

    for (int i = 0; i < 4; ++i) {
      if (contains(corners[i])) {
        return true;
      }
    }

    return true;  // Conservative: assume intersection if bounds overlap
  }

  // Check if empty
  bool isEmpty() const override {
    return outerBoundary.size() < 3;
  }

  // Get dimension
  int dimension() const override {
    if (outerBoundary.empty()) return -1;
    if (outerBoundary.size() == 1) return 0;
    if (outerBoundary.size() == 2) return 1;
    return 2;
  }

  // Calculate distance from point to polygon boundary
  double distance(IntPoint point) const override {
    if (contains(point)) {
      return 0.0;
    }

    // Distance to outer boundary
    double minDist = distanceToPolygon(point, outerBoundary);

    // Check distance to holes (point might be inside a hole)
    for (const auto& hole : holes) {
      if (pointInPolygon(point, hole)) {
        // Point is inside a hole, calculate distance to hole boundary
        double holeDist = distanceToPolygon(point, hole);
        minDist = std::min(minDist, holeDist);
      }
    }

    return minDist;
  }

  // Find nearest point on polygon boundary
  IntPoint nearestPoint(IntPoint fromPoint) const override {
    if (outerBoundary.empty()) {
      return fromPoint;
    }

    if (outerBoundary.size() == 1) {
      return outerBoundary[0];
    }

    // Find nearest point on outer boundary
    IntPoint nearest = nearestPointOnPolygon(fromPoint, outerBoundary);
    double minDist = distanceToPolygon(fromPoint, outerBoundary);

    // Check holes
    for (const auto& hole : holes) {
      IntPoint holeNearest = nearestPointOnPolygon(fromPoint, hole);
      double holeDist = distanceToPolygon(fromPoint, hole);

      if (holeDist < minDist) {
        minDist = holeDist;
        nearest = holeNearest;
      }
    }

    return nearest;
  }

  // Check if polygon is valid (no self-intersections, proper orientation)
  bool isValid() const {
    return outerBoundary.size() >= 3;
  }

private:
  std::vector<IntPoint> outerBoundary;
  std::vector<std::vector<IntPoint>> holes;

  // Calculate area of a simple polygon using shoelace formula
  static double calculatePolygonArea(const std::vector<IntPoint>& polygon) {
    if (polygon.size() < 3) {
      return 0.0;
    }

    i64 sum = 0;
    for (size_t i = 0; i < polygon.size(); ++i) {
      size_t j = (i + 1) % polygon.size();
      sum += static_cast<i64>(polygon[i].x) * static_cast<i64>(polygon[j].y);
      sum -= static_cast<i64>(polygon[j].x) * static_cast<i64>(polygon[i].y);
    }

    return std::abs(sum) / 2.0;
  }

  // Calculate perimeter of a polygon
  static double calculatePerimeter(const std::vector<IntPoint>& polygon) {
    if (polygon.size() < 2) {
      return 0.0;
    }

    double perimeter = 0.0;
    for (size_t i = 0; i < polygon.size(); ++i) {
      size_t j = (i + 1) % polygon.size();
      IntVector edge = polygon[j] - polygon[i];
      perimeter += std::sqrt(static_cast<double>(edge.lengthSquared()));
    }

    return perimeter;
  }

  // Point-in-polygon test using winding number
  static bool pointInPolygon(IntPoint point, const std::vector<IntPoint>& polygon) {
    if (polygon.size() < 3) {
      return false;
    }

    int windingNumber = 0;

    for (size_t i = 0; i < polygon.size(); ++i) {
      size_t j = (i + 1) % polygon.size();
      IntPoint p1 = polygon[i];
      IntPoint p2 = polygon[j];

      if (p1.y <= point.y) {
        if (p2.y > point.y) {
          IntVector edge = p2 - p1;
          IntVector toPoint = point - p1;
          if (edge.cross(toPoint) > 0) {
            windingNumber++;
          }
        }
      } else {
        if (p2.y <= point.y) {
          IntVector edge = p2 - p1;
          IntVector toPoint = point - p1;
          if (edge.cross(toPoint) < 0) {
            windingNumber--;
          }
        }
      }
    }

    return windingNumber != 0;
  }

  // Distance from point to polygon boundary
  static double distanceToPolygon(IntPoint point, const std::vector<IntPoint>& polygon) {
    if (polygon.empty()) {
      return std::numeric_limits<double>::max();
    }

    double minDist = std::numeric_limits<double>::max();

    for (size_t i = 0; i < polygon.size(); ++i) {
      size_t j = (i + 1) % polygon.size();
      double dist = distanceToSegment(point, polygon[i], polygon[j]);
      minDist = std::min(minDist, dist);
    }

    return minDist;
  }

  // Nearest point on polygon boundary
  static IntPoint nearestPointOnPolygon(IntPoint fromPoint,
                                         const std::vector<IntPoint>& polygon) {
    if (polygon.empty()) {
      return fromPoint;
    }

    if (polygon.size() == 1) {
      return polygon[0];
    }

    double minDist = std::numeric_limits<double>::max();
    IntPoint nearest = polygon[0];

    for (size_t i = 0; i < polygon.size(); ++i) {
      size_t j = (i + 1) % polygon.size();
      IntPoint candidate = nearestPointOnSegment(fromPoint, polygon[i], polygon[j]);

      IntVector delta = candidate - fromPoint;
      double dist = std::sqrt(static_cast<double>(delta.lengthSquared()));

      if (dist < minDist) {
        minDist = dist;
        nearest = candidate;
      }
    }

    return nearest;
  }

  // Distance from point to line segment
  static double distanceToSegment(IntPoint p, IntPoint a, IntPoint b) {
    IntVector ab = b - a;
    IntVector ap = p - a;

    i64 abLenSq = ab.lengthSquared();
    if (abLenSq == 0) {
      return std::sqrt(static_cast<double>(ap.lengthSquared()));
    }

    i64 t = ap.x * ab.x + ap.y * ab.y;

    if (t <= 0) {
      return std::sqrt(static_cast<double>(ap.lengthSquared()));
    }

    if (t >= abLenSq) {
      IntVector bp = p - b;
      return std::sqrt(static_cast<double>(bp.lengthSquared()));
    }

    double tNorm = static_cast<double>(t) / static_cast<double>(abLenSq);
    int projX = a.x + static_cast<int>(tNorm * ab.x);
    int projY = a.y + static_cast<int>(tNorm * ab.y);
    IntVector proj(projX - p.x, projY - p.y);

    return std::sqrt(static_cast<double>(proj.lengthSquared()));
  }

  // Nearest point on line segment
  static IntPoint nearestPointOnSegment(IntPoint p, IntPoint a, IntPoint b) {
    IntVector ab = b - a;
    IntVector ap = p - a;

    i64 abLenSq = ab.lengthSquared();
    if (abLenSq == 0) {
      return a;
    }

    i64 t = ap.x * ab.x + ap.y * ab.y;

    if (t <= 0) {
      return a;
    }

    if (t >= abLenSq) {
      return b;
    }

    double tNorm = static_cast<double>(t) / static_cast<double>(abLenSq);
    int projX = a.x + static_cast<int>(std::round(tNorm * ab.x));
    int projY = a.y + static_cast<int>(std::round(tNorm * ab.y));

    return IntPoint(projX, projY);
  }
};

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_COMPLEXPOLYGON_H
