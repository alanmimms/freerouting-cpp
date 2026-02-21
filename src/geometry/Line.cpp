#include "geometry/Line.h"
#include <cmath>
#include <cstdio>

namespace freerouting {

// ========== Factory Methods ==========

Line Line::fromPoints(const IntPoint& p1, const IntPoint& p2) {
  // Line from p1 to p2
  // Direction vector: (dx, dy) = (p2.x - p1.x, p2.y - p1.y)
  // Normal vector (perpendicular, 90° CCW): (-dy, dx)
  // Line equation: -dy * (x - p1.x) + dx * (y - p1.y) = 0
  // Expanded: -dy*x + dy*p1.x + dx*y - dx*p1.y = 0
  // Standard form: ax + by + c = 0 where a = -dy, b = dx, c = dy*p1.x - dx*p1.y

  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;

  double a = -static_cast<double>(dy);
  double b = static_cast<double>(dx);
  double c = static_cast<double>(dy * p1.x - dx * p1.y);

  return Line(a, b, c);
}

Line Line::fromPoints(const FloatPoint& p1, const FloatPoint& p2) {
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;

  double a = -dy;
  double b = dx;
  double c = dy * p1.x - dx * p1.y;

  return Line(a, b, c);
}

Line Line::horizontal(int yCoord) {
  // Horizontal line at y = yCoord
  // Equation: y - yCoord = 0
  // Standard form: 0*x + 1*y + (-yCoord) = 0
  // Points above (y > yCoord) have 0*x + 1*y + (-yCoord) > 0 (left side)
  return Line(0.0, 1.0, -static_cast<double>(yCoord));
}

Line Line::vertical(int xCoord) {
  // Vertical line at x = xCoord
  // Equation: x - xCoord = 0
  // Standard form: 1*x + 0*y + (-xCoord) = 0
  // Points to the right (x > xCoord) have 1*x + 0*y + (-xCoord) > 0 (left side)
  return Line(1.0, 0.0, -static_cast<double>(xCoord));
}

// ========== Side Queries ==========

double Line::sideOf(const IntPoint& point) const {
  // Evaluate ax + by + c
  return a * static_cast<double>(point.x) +
         b * static_cast<double>(point.y) + c;
}

double Line::sideOf(const FloatPoint& point) const {
  return a * point.x + b * point.y + c;
}

bool Line::isOnLine(const IntPoint& point, double epsilon) const {
  return std::abs(sideOf(point)) < epsilon;
}

// ========== Normalization ==========

void Line::normalize() {
  double length = std::sqrt(a * a + b * b);
  if (length > 1e-10) {
    a /= length;
    b /= length;
    c /= length;
  }
}

Line Line::normalized() const {
  Line result = *this;
  result.normalize();
  return result;
}

// ========== Intersection ==========

bool Line::intersect(const Line& other, FloatPoint& outPoint) const {
  // Solve system:
  //   a1*x + b1*y + c1 = 0
  //   a2*x + b2*y + c2 = 0
  //
  // Using Cramer's rule:
  //   det = a1*b2 - a2*b1
  //   x = (b1*c2 - b2*c1) / det
  //   y = (a2*c1 - a1*c2) / det

  double det = a * other.b - other.a * b;

  if (std::abs(det) < 1e-10) {
    // Lines are parallel or coincident
    return false;
  }

  outPoint.x = (b * other.c - other.b * c) / det;
  outPoint.y = (other.a * c - a * other.c) / det;

  return true;
}

// ========== Comparison ==========

bool Line::equals(const Line& other, double epsilon) const {
  // Lines are equal if they represent the same line
  // Handle scaling: k*a, k*b, k*c represent the same line

  // Normalize both and compare
  Line n1 = this->normalized();
  Line n2 = other.normalized();

  // Check if coefficients match (accounting for opposite directions)
  bool sameDir = std::abs(n1.a - n2.a) < epsilon &&
                 std::abs(n1.b - n2.b) < epsilon &&
                 std::abs(n1.c - n2.c) < epsilon;

  bool oppositeDir = std::abs(n1.a + n2.a) < epsilon &&
                     std::abs(n1.b + n2.b) < epsilon &&
                     std::abs(n1.c + n2.c) < epsilon;

  return sameDir || oppositeDir;
}

// ========== String Representation ==========

const char* Line::toString() const {
  static char buffer[256];
  snprintf(buffer, sizeof(buffer), "Line(%.3fx + %.3fy + %.3f = 0)", a, b, c);
  return buffer;
}

} // namespace freerouting
