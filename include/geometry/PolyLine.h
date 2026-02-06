#ifndef FREEROUTING_GEOMETRY_POLYLINE_H
#define FREEROUTING_GEOMETRY_POLYLINE_H

#include "geometry/Shape.h"
#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include <vector>
#include <cmath>
#include <algorithm>

namespace freerouting {

// Open or closed polyline (sequence of connected line segments)
// Used for board outlines, graphic lines, and trace paths
class PolyLine : public Shape {
public:
  // Construct empty polyline
  PolyLine() : closed(false), lineWidth(0) {}

  // Construct polyline from points
  PolyLine(const std::vector<IntPoint>& pts, bool isClosed = false, int width = 0)
    : points(pts), closed(isClosed), lineWidth(width) {}

  // Get number of points
  size_t pointCount() const { return points.size(); }

  // Get point at index
  IntPoint getPoint(size_t index) const {
    return index < points.size() ? points[index] : IntPoint(0, 0);
  }

  // Get all points
  const std::vector<IntPoint>& getPoints() const { return points; }

  // Check if polyline is closed
  bool isClosed() const { return closed; }

  // Get line width (for thick lines)
  int getLineWidth() const { return lineWidth; }

  // Add point to polyline
  void addPoint(IntPoint point) {
    points.push_back(point);
  }

  // Set whether polyline is closed
  void setClosed(bool shouldClose) {
    closed = shouldClose;
  }

  // Set line width
  void setLineWidth(int width) {
    lineWidth = width;
  }

  // Get bounding box
  IntBox getBoundingBox() const override {
    if (points.empty()) {
      return IntBox(0, 0, 0, 0);
    }

    int minX = points[0].x;
    int maxX = points[0].x;
    int minY = points[0].y;
    int maxY = points[0].y;

    for (size_t i = 1; i < points.size(); ++i) {
      minX = std::min(minX, points[i].x);
      maxX = std::max(maxX, points[i].x);
      minY = std::min(minY, points[i].y);
      maxY = std::max(maxY, points[i].y);
    }

    // Expand by half line width if present
    int halfWidth = lineWidth / 2;
    return IntBox(minX - halfWidth, minY - halfWidth,
                  maxX + halfWidth, maxY + halfWidth);
  }

  // Calculate area (zero for open polyline, shoelace for closed)
  double area() const override {
    if (!closed || points.size() < 3) {
      return 0.0;
    }

    i64 sum = 0;
    for (size_t i = 0; i < points.size(); ++i) {
      size_t j = (i + 1) % points.size();
      sum += static_cast<i64>(points[i].x) * static_cast<i64>(points[j].y);
      sum -= static_cast<i64>(points[j].x) * static_cast<i64>(points[i].y);
    }

    return std::abs(sum) / 2.0;
  }

  // Calculate total length
  double circumference() const override {
    if (points.size() < 2) {
      return 0.0;
    }

    double length = 0.0;
    for (size_t i = 0; i < points.size() - 1; ++i) {
      IntVector segment = points[i + 1] - points[i];
      length += std::sqrt(static_cast<double>(segment.lengthSquared()));
    }

    // Add closing segment if closed
    if (closed && points.size() >= 2) {
      IntVector closingSegment = points[0] - points.back();
      length += std::sqrt(static_cast<double>(closingSegment.lengthSquared()));
    }

    return length;
  }

  // Check if point is inside closed polyline (winding number test)
  bool contains(IntPoint point) const override {
    if (!closed || points.size() < 3) {
      return false;
    }

    // Winding number algorithm
    int windingNumber = 0;

    for (size_t i = 0; i < points.size(); ++i) {
      size_t j = (i + 1) % points.size();
      IntPoint p1 = points[i];
      IntPoint p2 = points[j];

      if (p1.y <= point.y) {
        if (p2.y > point.y) {
          // Upward crossing
          IntVector edge = p2 - p1;
          IntVector toPoint = point - p1;
          if (edge.cross(toPoint) > 0) {
            windingNumber++;
          }
        }
      } else {
        if (p2.y <= point.y) {
          // Downward crossing
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

  // Check if point is strictly inside
  bool containsInside(IntPoint point) const override {
    // For polylines, we don't distinguish between boundary and interior
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

    // Check if any point is inside box
    for (const auto& point : points) {
      if (box.contains(point)) {
        return true;
      }
    }

    // Check if any box corner is inside closed polyline
    if (closed && points.size() >= 3) {
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
    }

    return true;  // Conservative: assume intersection if bounds overlap
  }

  // Check if empty
  bool isEmpty() const override {
    return points.empty();
  }

  // Get dimension
  int dimension() const override {
    if (points.empty()) return -1;
    if (points.size() == 1) return 0;
    if (!closed || points.size() < 3) return 1;  // Line
    return 2;  // Closed polygon
  }

  // Calculate distance from point to polyline
  double distance(IntPoint point) const override {
    if (points.empty()) {
      return std::numeric_limits<double>::max();
    }

    if (points.size() == 1) {
      IntVector delta = point - points[0];
      return std::sqrt(static_cast<double>(delta.lengthSquared()));
    }

    double minDist = std::numeric_limits<double>::max();

    // Check distance to all segments
    size_t segmentCount = closed ? points.size() : points.size() - 1;
    for (size_t i = 0; i < segmentCount; ++i) {
      size_t j = (i + 1) % points.size();
      double dist = distanceToSegment(point, points[i], points[j]);
      minDist = std::min(minDist, dist);
    }

    return minDist;
  }

  // Find nearest point on polyline
  IntPoint nearestPoint(IntPoint fromPoint) const override {
    if (points.empty()) {
      return fromPoint;
    }

    if (points.size() == 1) {
      return points[0];
    }

    double minDist = std::numeric_limits<double>::max();
    IntPoint nearest = points[0];

    size_t segmentCount = closed ? points.size() : points.size() - 1;
    for (size_t i = 0; i < segmentCount; ++i) {
      size_t j = (i + 1) % points.size();
      IntPoint candidate = nearestPointOnSegment(fromPoint, points[i], points[j]);

      IntVector delta = candidate - fromPoint;
      double dist = std::sqrt(static_cast<double>(delta.lengthSquared()));

      if (dist < minDist) {
        minDist = dist;
        nearest = candidate;
      }
    }

    return nearest;
  }

  // Get segment count
  size_t segmentCount() const {
    if (points.size() < 2) return 0;
    return closed ? points.size() : points.size() - 1;
  }

  // Get segment endpoints
  void getSegment(size_t index, IntPoint& p1, IntPoint& p2) const {
    if (index >= segmentCount()) {
      p1 = p2 = IntPoint(0, 0);
      return;
    }

    p1 = points[index];
    p2 = points[(index + 1) % points.size()];
  }

private:
  std::vector<IntPoint> points;
  bool closed;
  int lineWidth;

  // Calculate distance from point to line segment
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

#endif // FREEROUTING_GEOMETRY_POLYLINE_H
