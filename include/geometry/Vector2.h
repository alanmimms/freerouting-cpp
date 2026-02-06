#ifndef FREEROUTING_GEOMETRY_VECTOR2_H
#define FREEROUTING_GEOMETRY_VECTOR2_H

#include "core/Types.h"
#include "Side.h"
#include <cmath>
#include <algorithm>

namespace freerouting {

// Critical integer limit (2^25)
// Product of two CRIT_INT values fits in double mantissa with room for addition
constexpr int kCritInt = 33554432; // 2^25

// Forward declarations for cross-type operations
template<typename T> struct Vector2;

// 2D vector/point template
// Used for both points and vectors in the plane
// Template parameter must be ALLUPPERCASE per coding standards
template<typename T>
struct Vector2 {
  T x;
  T y;

  // Default constructor - zero vector/point
  constexpr Vector2() : x(0), y(0) {}

  // Constructor from coordinates
  constexpr Vector2(T xVal, T yVal) : x(xVal), y(yVal) {}

  // Copy constructor (implicit is fine, but explicit for clarity)
  constexpr Vector2(const Vector2& other) = default;

  // Assignment operator
  constexpr Vector2& operator=(const Vector2& other) = default;

  // Equality comparison
  constexpr bool operator==(const Vector2& other) const {
    return x == other.x && y == other.y;
  }

  constexpr bool operator!=(const Vector2& other) const {
    return !(*this == other);
  }

  // Ordering (for use in std::map/set)
  constexpr bool operator<(const Vector2& other) const {
    if (x != other.x) return x < other.x;
    return y < other.y;
  }

  // Check if zero vector
  constexpr bool isZero() const {
    return x == T(0) && y == T(0);
  }

  // Vector arithmetic
  constexpr Vector2 operator+(const Vector2& other) const {
    return Vector2(x + other.x, y + other.y);
  }

  constexpr Vector2 operator-(const Vector2& other) const {
    return Vector2(x - other.x, y - other.y);
  }

  constexpr Vector2 operator-() const {
    return Vector2(-x, -y);
  }

  constexpr Vector2& operator+=(const Vector2& other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  constexpr Vector2& operator-=(const Vector2& other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  // Scalar multiplication
  constexpr Vector2 operator*(T scalar) const {
    return Vector2(x * scalar, y * scalar);
  }

  constexpr Vector2& operator*=(T scalar) {
    x *= scalar;
    y *= scalar;
    return *this;
  }

  // Dot product
  constexpr T dot(const Vector2& other) const {
    return x * other.x + y * other.y;
  }

  // Cross product (returns scalar for 2D - z component of 3D cross product)
  constexpr T cross(const Vector2& other) const {
    return x * other.y - y * other.x;
  }

  // Determinant (same as cross product for 2D vectors)
  constexpr T determinant(const Vector2& other) const {
    return cross(other);
  }

  // Determine which side of line from origin to 'other' this vector is on
  // Returns: OnTheLeft if this is left of the line
  //          OnTheRight if this is right of the line
  //          Collinear if this is on the line
  constexpr Side sideOf(const Vector2& other) const {
    return sideFrom(other.cross(*this));
  }

  // Length squared (avoids sqrt for performance)
  constexpr T lengthSquared() const {
    return x * x + y * y;
  }

  // Length (only for floating point types)
  template<typename U = T>
  typename std::enable_if<std::is_floating_point<U>::value, U>::type
  length() const {
    return std::sqrt(lengthSquared());
  }

  // Normalize (only for floating point types)
  template<typename U = T>
  typename std::enable_if<std::is_floating_point<U>::value, Vector2<U>>::type
  normalized() const {
    U len = length();
    if (len > U(0)) {
      return Vector2<U>(x / len, y / len);
    }
    return Vector2<U>(U(0), U(0));
  }

  // Check if vector is orthogonal (horizontal or vertical)
  constexpr bool isOrthogonal() const {
    return x == T(0) || y == T(0);
  }

  // Check if vector is diagonal (|x| == |y|)
  constexpr bool isDiagonal() const {
    return std::abs(x) == std::abs(y);
  }

  // Rotate 90 degrees counterclockwise
  // factor: 0=0°, 1=90°, 2=180°, 3=270°
  constexpr Vector2 rotate90(int factor) const {
    int n = factor % 4;
    if (n < 0) n += 4;

    switch (n) {
      case 0: return Vector2(x, y);        // 0°
      case 1: return Vector2(-y, x);       // 90°
      case 2: return Vector2(-x, -y);      // 180°
      case 3: return Vector2(y, -x);       // 270°
      default: return Vector2(x, y);
    }
  }

  // Convert to floating point vector
  constexpr Vector2<double> toDouble() const {
    return Vector2<double>(static_cast<double>(x), static_cast<double>(y));
  }

  // Convert to integer vector (truncating)
  constexpr Vector2<int> toInt() const {
    return Vector2<int>(static_cast<int>(x), static_cast<int>(y));
  }

  // Manhattan distance (L1 norm)
  constexpr T manhattanLength() const {
    return std::abs(x) + std::abs(y);
  }

  // Chebyshev distance (L∞ norm)
  constexpr T chebyshevLength() const {
    return std::max(std::abs(x), std::abs(y));
  }
};

// Scalar multiplication (scalar on left)
template<typename T>
constexpr Vector2<T> operator*(T scalar, const Vector2<T>& v) {
  return v * scalar;
}

// Type aliases for common uses
using IntPoint = Vector2<int>;
using IntVector = Vector2<int>;
using FloatPoint = Vector2<double>;
using FloatVector = Vector2<double>;

// Common constants
namespace constants {
  constexpr IntPoint kZeroInt{0, 0};
  constexpr FloatPoint kZeroFloat{0.0, 0.0};
}

} // namespace freerouting

#endif // FREEROUTING_GEOMETRY_VECTOR2_H
