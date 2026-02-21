#include "geometry/IntBoxShape.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace freerouting {

// ========== Constructors ==========

IntBoxShape::IntBoxShape(const IntBox& box) : box(box) {}

IntBoxShape::IntBoxShape(int minX, int minY, int maxX, int maxY)
  : box(minX, minY, maxX, maxY) {}

IntBoxShape::IntBoxShape(const IntBoxShape& other) : box(other.box) {}

// ========== TileShape Interface ==========

int IntBoxShape::dimension() const {
  if (box.isEmpty()) {
    return -1;  // Empty
  }
  if (box.width() == 0 && box.height() == 0) {
    return 0;  // Point
  }
  if (box.width() == 0 || box.height() == 0) {
    return 1;  // Line
  }
  return 2;  // Area
}

bool IntBoxShape::isEmpty() const {
  return box.isEmpty();
}

bool IntBoxShape::contains(IntPoint point) const {
  return box.contains(point);
}

IntBox IntBoxShape::getBoundingBox() const {
  return box;
}

const char* IntBoxShape::getTypeName() const {
  return "IntBoxShape";
}

bool IntBoxShape::equals(const TileShape& other) const {
  const IntBoxShape* otherBox = dynamic_cast<const IntBoxShape*>(&other);
  if (!otherBox) {
    return false;
  }
  return box == otherBox->box;
}

// ========== Intersection ==========

TileShape* IntBoxShape::intersection(const TileShape& other) const {
  // Try specialized IntBoxShape intersection
  const IntBoxShape* otherBox = dynamic_cast<const IntBoxShape*>(&other);
  if (otherBox) {
    return intersectionWithBox(*otherBox);
  }

  // TODO: General TileShape intersection (for IntOctagon, Simplex)
  // For now, return nullptr for non-box shapes
  return nullptr;
}

IntBoxShape* IntBoxShape::intersectionWithBox(const IntBoxShape& other) const {
  IntBox result = box.intersection(other.box);
  if (result.isEmpty()) {
    return nullptr;
  }
  return new IntBoxShape(result);
}

bool IntBoxShape::intersects(const TileShape& other) const {
  const IntBoxShape* otherBox = dynamic_cast<const IntBoxShape*>(&other);
  if (otherBox) {
    return intersectsBox(*otherBox);
  }

  // TODO: General TileShape intersection test
  return false;
}

bool IntBoxShape::intersectsBox(const IntBoxShape& other) const {
  // Check for 2D overlap (not just touching edges)
  IntBox overlap = box.intersection(other.box);
  if (overlap.isEmpty()) {
    return false;
  }

  // If intersection is 1D (shared edge) or 0D (shared corner), not an intersection
  return overlap.width() > 0 && overlap.height() > 0;
}

// ========== Half-Plane Intersection ==========

TileShape* IntBoxShape::intersectionWithHalfplane(const Line& line) const {
  // Check if line is axis-aligned (horizontal or vertical)
  constexpr double epsilon = 1e-6;

  bool isHorizontal = std::abs(line.a) < epsilon;  // a ≈ 0 means horizontal
  bool isVertical = std::abs(line.b) < epsilon;    // b ≈ 0 means vertical

  if (isHorizontal || isVertical) {
    return clipWithAxisAlignedLine(line);
  }

  // TODO: General line clipping (Sutherland-Hodgman or similar)
  // For now, handle only axis-aligned lines (sufficient for 90° routing)
  // General case needed for 45° routing (IntOctagon)
  return new IntBoxShape(*this);  // Return copy as fallback
}

IntBoxShape* IntBoxShape::clipWithAxisAlignedLine(const Line& line) const {
  constexpr double epsilon = 1e-6;

  if (std::abs(line.a) < epsilon) {
    // Horizontal line: y = -c/b
    double yCoord = -line.c / line.b;

    // Line equation: 0*x + b*y + c = 0
    // Left side (keep): b*y + c >= 0, or y >= -c/b if b > 0, y <= -c/b if b < 0

    int yClip = static_cast<int>(std::round(yCoord));

    if (line.b > 0) {
      // Keep y >= yClip (above or on the line)
      int newMinY = std::max(box.ll.y, yClip);
      if (newMinY > box.ur.y) {
        return nullptr;  // Empty result
      }
      return new IntBoxShape(box.ll.x, newMinY, box.ur.x, box.ur.y);
    } else {
      // Keep y <= yClip (below or on the line)
      int newMaxY = std::min(box.ur.y, yClip);
      if (newMaxY < box.ll.y) {
        return nullptr;  // Empty result
      }
      return new IntBoxShape(box.ll.x, box.ll.y, box.ur.x, newMaxY);
    }
  } else if (std::abs(line.b) < epsilon) {
    // Vertical line: x = -c/a
    double xCoord = -line.c / line.a;

    // Line equation: a*x + 0*y + c = 0
    // Left side (keep): a*x + c >= 0, or x >= -c/a if a > 0, x <= -c/a if a < 0

    int xClip = static_cast<int>(std::round(xCoord));

    if (line.a > 0) {
      // Keep x >= xClip (right of or on the line)
      int newMinX = std::max(box.ll.x, xClip);
      if (newMinX > box.ur.x) {
        return nullptr;  // Empty result
      }
      return new IntBoxShape(newMinX, box.ll.y, box.ur.x, box.ur.y);
    } else {
      // Keep x <= xClip (left of or on the line)
      int newMaxX = std::min(box.ur.x, xClip);
      if (newMaxX < box.ll.x) {
        return nullptr;  // Empty result
      }
      return new IntBoxShape(box.ll.x, box.ll.y, newMaxX, box.ur.y);
    }
  }

  // Should not reach here if called correctly
  return new IntBoxShape(*this);
}

// ========== Border Operations ==========

int IntBoxShape::borderLineCount() const {
  return 4;
}

Line IntBoxShape::borderLine(int index) const {
  // Counter-clockwise starting from bottom:
  // 0: Bottom (y = minY, pointing right)
  // 1: Right  (x = maxX, pointing up)
  // 2: Top    (y = maxY, pointing left)
  // 3: Left   (x = minX, pointing down)

  switch (index) {
    case 0: {
      // Bottom edge: from (ll.x, ll.y) to (ur.x, ll.y)
      IntPoint p1(box.ll.x, box.ll.y);
      IntPoint p2(box.ur.x, box.ll.y);
      return Line::fromPoints(p1, p2);
    }
    case 1: {
      // Right edge: from (ur.x, ll.y) to (ur.x, ur.y)
      IntPoint p1(box.ur.x, box.ll.y);
      IntPoint p2(box.ur.x, box.ur.y);
      return Line::fromPoints(p1, p2);
    }
    case 2: {
      // Top edge: from (ur.x, ur.y) to (ll.x, ur.y)
      IntPoint p1(box.ur.x, box.ur.y);
      IntPoint p2(box.ll.x, box.ur.y);
      return Line::fromPoints(p1, p2);
    }
    case 3: {
      // Left edge: from (ll.x, ur.y) to (ll.x, ll.y)
      IntPoint p1(box.ll.x, box.ur.y);
      IntPoint p2(box.ll.x, box.ll.y);
      return Line::fromPoints(p1, p2);
    }
    default:
      return Line::horizontal(0);  // Invalid index, return dummy
  }
}

IntPoint IntBoxShape::corner(int index) const {
  switch (index) {
    case 0: return IntPoint(box.ll.x, box.ll.y);  // Bottom-left
    case 1: return IntPoint(box.ur.x, box.ll.y);  // Bottom-right
    case 2: return IntPoint(box.ur.x, box.ur.y);  // Top-right
    case 3: return IntPoint(box.ll.x, box.ur.y);  // Top-left
    default: return IntPoint(box.ll.x, box.ll.y);
  }
}

int* IntBoxShape::touchingSides(const TileShape& other) const {
  const IntBoxShape* otherBox = dynamic_cast<const IntBoxShape*>(&other);
  if (!otherBox) {
    return nullptr;  // TODO: Handle non-box shapes
  }

  return findTouchingEdges(*otherBox);
}

int* IntBoxShape::findTouchingEdges(const IntBoxShape& other) const {
  std::vector<int> touching;

  // Check each edge for 1D intersection
  IntBox overlap = box.intersection(other.box);

  if (overlap.isEmpty()) {
    return nullptr;  // No intersection
  }

  // 2D intersection - not a touch
  if (overlap.width() > 0 && overlap.height() > 0) {
    return nullptr;
  }

  // 1D or 0D intersection - find which edges touch
  // Check bottom edge (y = ll.y)
  if (box.ll.y == other.box.ur.y || box.ll.y == other.box.ll.y) {
    if (overlap.width() > 0 || (overlap.ll.x == overlap.ur.x &&
        overlap.ll.x >= box.ll.x && overlap.ll.x <= box.ur.x)) {
      touching.push_back(0);
    }
  }

  // Check right edge (x = ur.x)
  if (box.ur.x == other.box.ll.x || box.ur.x == other.box.ur.x) {
    if (overlap.height() > 0 || (overlap.ll.y == overlap.ur.y &&
        overlap.ll.y >= box.ll.y && overlap.ll.y <= box.ur.y)) {
      touching.push_back(1);
    }
  }

  // Check top edge (y = ur.y)
  if (box.ur.y == other.box.ll.y || box.ur.y == other.box.ur.y) {
    if (overlap.width() > 0 || (overlap.ll.x == overlap.ur.x &&
        overlap.ll.x >= box.ll.x && overlap.ll.x <= box.ur.x)) {
      touching.push_back(2);
    }
  }

  // Check left edge (x = ll.x)
  if (box.ll.x == other.box.ur.x || box.ll.x == other.box.ll.x) {
    if (overlap.height() > 0 || (overlap.ll.y == overlap.ur.y &&
        overlap.ll.y >= box.ll.y && overlap.ll.y <= box.ur.y)) {
      touching.push_back(3);
    }
  }

  if (touching.empty()) {
    return nullptr;
  }

  // Allocate and return array
  int* result = new int[touching.size() + 1];
  for (size_t i = 0; i < touching.size(); i++) {
    result[i] = touching[i];
  }
  result[touching.size()] = -1;  // Sentinel
  return result;
}

// ========== Distance Queries ==========

double IntBoxShape::distanceToLeft(const Line& line) const {
  // Find the corner farthest to the left of the line
  return maxCornerDistanceToLeft(line);
}

double IntBoxShape::maxCornerDistanceToLeft(const Line& line) const {
  double maxDist = -std::numeric_limits<double>::infinity();

  for (int i = 0; i < 4; i++) {
    IntPoint c = corner(i);
    double dist = line.sideOf(c);
    if (dist > maxDist) {
      maxDist = dist;
    }
  }

  return maxDist;
}

// ========== Line Intersection Tests ==========

bool IntBoxShape::intersectsInterior(const Line& line) const {
  return linePassesThroughInterior(line);
}

bool IntBoxShape::linePassesThroughInterior(const Line& line) const {
  // Check if line passes through the interior of the box
  // A line passes through interior if:
  // 1. Some corners are on left side, some on right side
  // 2. Or line passes through box (some corners on each side)

  int leftCount = 0;
  int rightCount = 0;

  for (int i = 0; i < 4; i++) {
    IntPoint c = corner(i);
    double side = line.sideOf(c);
    if (side > 1e-6) {
      leftCount++;
    } else if (side < -1e-6) {
      rightCount++;
    }
  }

  // Line intersects interior if corners are on both sides
  return leftCount > 0 && rightCount > 0;
}

// ========== Shape Interface Implementations ==========

double IntBoxShape::area() const {
  if (dimension() < 2) return 0.0;
  return static_cast<double>(box.width()) * static_cast<double>(box.height());
}

double IntBoxShape::circumference() const {
  if (dimension() < 2) return 0.0;
  return 2.0 * (static_cast<double>(box.width()) + static_cast<double>(box.height()));
}

bool IntBoxShape::containsInside(IntPoint point) const {
  // Strictly inside means not on boundary
  return point.x > box.ll.x && point.x < box.ur.x &&
         point.y > box.ll.y && point.y < box.ur.y;
}

bool IntBoxShape::intersects(const Shape& other) const {
  // Try to cast to TileShape for specialized handling
  const TileShape* tileShape = dynamic_cast<const TileShape*>(&other);
  if (tileShape) {
    return intersects(*tileShape);
  }

  // Fallback: use bounding box intersection
  return box.intersects(other.getBoundingBox());
}

bool IntBoxShape::intersects(const IntBox& otherBox) const {
  return box.intersects(otherBox);
}

double IntBoxShape::distance(IntPoint point) const {
  // Distance to nearest point on box boundary
  if (contains(point)) {
    return 0.0;  // Point is inside or on boundary
  }

  // Find nearest point and calculate distance
  IntPoint nearest = nearestPoint(point);
  int dx = point.x - nearest.x;
  int dy = point.y - nearest.y;
  return std::sqrt(static_cast<double>(dx * dx + dy * dy));
}

IntPoint IntBoxShape::nearestPoint(IntPoint fromPoint) const {
  // Clamp point to box bounds
  int nearX = std::max(box.ll.x, std::min(fromPoint.x, box.ur.x));
  int nearY = std::max(box.ll.y, std::min(fromPoint.y, box.ur.y));
  return IntPoint(nearX, nearY);
}

} // namespace freerouting
