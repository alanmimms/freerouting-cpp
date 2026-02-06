#ifndef FREEROUTING_GEOMETRY_CONVEXPOLYGON_H
#define FREEROUTING_GEOMETRY_CONVEXPOLYGON_H

#include "geometry/Shape.h"
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include "geometry/Side.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace freerouting {

// Convex polygon with integer coordinates
// Points must be in counter-clockwise order
// Duplicate and collinear points are removed during construction
class ConvexPolygon : public Shape {
public:
  // Construct from vector of points (removes duplicates and collinear points)
  explicit ConvexPolygon(const std::vector<IntPoint>& points) {
    if (points.empty()) {
      return;
    }

    // First pass: remove consecutive duplicates
    std::vector<IntPoint> noDuplicates;
    noDuplicates.reserve(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
      if (noDuplicates.empty() || noDuplicates.back() != points[i]) {
        noDuplicates.push_back(points[i]);
      }
    }

    if (noDuplicates.size() < 3) {
      vertices = std::move(noDuplicates);
      return;
    }

    // Second pass: remove collinear points
    // Check each point against its predecessor and successor (with wraparound)
    std::vector<IntPoint> cleaned;
    cleaned.reserve(noDuplicates.size());

    size_t n = noDuplicates.size();
    for (size_t i = 0; i < n; ++i) {
      size_t prev = (i + n - 1) % n;
      size_t next = (i + 1) % n;

      IntVector v1 = noDuplicates[i] - noDuplicates[prev];
      IntVector v2 = noDuplicates[next] - noDuplicates[i];

      // Keep point if not collinear with neighbors
      if (v1.cross(v2) != 0) {
        cleaned.push_back(noDuplicates[i]);
      }
    }

    vertices = std::move(cleaned);
  }

  // Get number of vertices
  size_t vertexCount() const { return vertices.size(); }

  // Get vertex at index
  IntPoint getVertex(size_t index) const {
    return index < vertices.size() ? vertices[index] : IntPoint(0, 0);
  }

  // Get all vertices
  const std::vector<IntPoint>& getVertices() const { return vertices; }

  // Get bounding box
  IntBox getBoundingBox() const override {
    if (vertices.empty()) {
      return IntBox(0, 0, 0, 0);
    }

    int minX = vertices[0].x;
    int maxX = vertices[0].x;
    int minY = vertices[0].y;
    int maxY = vertices[0].y;

    for (size_t i = 1; i < vertices.size(); ++i) {
      minX = std::min(minX, vertices[i].x);
      maxX = std::max(maxX, vertices[i].x);
      minY = std::min(minY, vertices[i].y);
      maxY = std::max(maxY, vertices[i].y);
    }

    return IntBox(minX, minY, maxX, maxY);
  }

  // Calculate area using shoelace formula
  double area() const override {
    if (vertices.size() < 3) {
      return 0.0;
    }

    i64 sum = 0;
    for (size_t i = 0; i < vertices.size(); ++i) {
      size_t j = (i + 1) % vertices.size();
      sum += static_cast<i64>(vertices[i].x) * static_cast<i64>(vertices[j].y);
      sum -= static_cast<i64>(vertices[j].x) * static_cast<i64>(vertices[i].y);
    }

    return std::abs(sum) / 2.0;
  }

  // Calculate perimeter
  double circumference() const override {
    if (vertices.size() < 2) {
      return 0.0;
    }

    double perimeter = 0.0;
    for (size_t i = 0; i < vertices.size(); ++i) {
      size_t j = (i + 1) % vertices.size();
      IntVector edge = vertices[j] - vertices[i];
      perimeter += std::sqrt(static_cast<double>(edge.lengthSquared()));
    }

    return perimeter;
  }

  // Check if point is inside polygon using winding number
  bool contains(IntPoint point) const override {
    if (vertices.size() < 3) {
      return false;
    }

    // For convex polygons, point is inside if it's on the same side
    // of all edges (assuming counter-clockwise winding)
    for (size_t i = 0; i < vertices.size(); ++i) {
      size_t j = (i + 1) % vertices.size();
      IntVector edge = vertices[j] - vertices[i];
      IntVector toPoint = point - vertices[i];

      // If point is on right side of any edge, it's outside
      if (edge.cross(toPoint) < 0) {
        return false;
      }
    }

    return true;
  }

  // Check if point is strictly inside
  bool containsInside(IntPoint point) const override {
    if (vertices.size() < 3) {
      return false;
    }

    for (size_t i = 0; i < vertices.size(); ++i) {
      size_t j = (i + 1) % vertices.size();
      IntVector edge = vertices[j] - vertices[i];
      IntVector toPoint = point - vertices[i];

      // If point is on or right side of any edge, it's not strictly inside
      if (edge.cross(toPoint) <= 0) {
        return false;
      }
    }

    return true;
  }

  // Check intersection with another shape (conservative bounding box test)
  bool intersects(const Shape& other) const override {
    return getBoundingBox().intersects(other.getBoundingBox());
  }

  // Check intersection with box
  bool intersects(const IntBox& box) const override {
    // Quick reject: if bounding boxes don't intersect
    if (!getBoundingBox().intersects(box)) {
      return false;
    }

    // Check if any polygon vertex is inside box
    for (const auto& vertex : vertices) {
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

    // If we get here, either they don't intersect or edges cross
    // For simplicity, assume they intersect if bounding boxes overlap
    return true;
  }

  // Check if empty
  bool isEmpty() const override {
    return vertices.size() < 3;
  }

  // Get dimension
  int dimension() const override {
    if (vertices.empty()) return -1;
    if (vertices.size() == 1) return 0;
    if (vertices.size() == 2) return 1;
    return 2;
  }

  // Calculate distance from point to polygon (approximate)
  double distance(IntPoint point) const override {
    if (contains(point)) {
      return 0.0;
    }

    double minDist = std::numeric_limits<double>::max();

    // Check distance to all edges
    for (size_t i = 0; i < vertices.size(); ++i) {
      size_t j = (i + 1) % vertices.size();
      double dist = distanceToSegment(point, vertices[i], vertices[j]);
      minDist = std::min(minDist, dist);
    }

    return minDist;
  }

  // Find nearest point on polygon boundary
  IntPoint nearestPoint(IntPoint fromPoint) const override {
    if (vertices.empty()) {
      return fromPoint;
    }

    if (vertices.size() == 1) {
      return vertices[0];
    }

    double minDist = std::numeric_limits<double>::max();
    IntPoint nearest = vertices[0];

    // Check all edges
    for (size_t i = 0; i < vertices.size(); ++i) {
      size_t j = (i + 1) % vertices.size();
      IntPoint candidate = nearestPointOnSegment(fromPoint, vertices[i], vertices[j]);

      IntVector delta = candidate - fromPoint;
      double dist = std::sqrt(static_cast<double>(delta.lengthSquared()));

      if (dist < minDist) {
        minDist = dist;
        nearest = candidate;
      }
    }

    return nearest;
  }

private:
  std::vector<IntPoint> vertices;

  // Calculate distance from point to line segment
  static double distanceToSegment(IntPoint p, IntPoint a, IntPoint b) {
    IntVector ab = b - a;
    IntVector ap = p - a;

    i64 abLenSq = ab.lengthSquared();
    if (abLenSq == 0) {
      // Segment is a point
      return std::sqrt(static_cast<double>(ap.lengthSquared()));
    }

    // Project p onto line ab
    i64 t = ap.x * ab.x + ap.y * ab.y;

    if (t <= 0) {
      // Closest to point a
      return std::sqrt(static_cast<double>(ap.lengthSquared()));
    }

    if (t >= abLenSq) {
      // Closest to point b
      IntVector bp = p - b;
      return std::sqrt(static_cast<double>(bp.lengthSquared()));
    }

    // Closest to point on segment
    double tNorm = static_cast<double>(t) / static_cast<double>(abLenSq);
    int projX = a.x + static_cast<int>(tNorm * ab.x);
    int projY = a.y + static_cast<int>(tNorm * ab.y);
    IntVector proj(projX - p.x, projY - p.y);

    return std::sqrt(static_cast<double>(proj.lengthSquared()));
  }

  // Find nearest point on line segment
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

#endif // FREEROUTING_GEOMETRY_CONVEXPOLYGON_H
