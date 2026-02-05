#ifndef FREEROUTING_CORE_FIXEDPOINT_H
#define FREEROUTING_CORE_FIXEDPOINT_H

#include "Types.h"

namespace freerouting {

// Fixed-point precision configuration
// Using 64-bit storage with configurable fractional bits
constexpr int kFixedPointFractionalBits = 16;
constexpr i64 kFixedPointScale = 1LL << kFixedPointFractionalBits; // 65536
constexpr i64 kFixedPointMask = kFixedPointScale - 1; // 65535

// Fixed-point type for geometric calculations
// Provides exact integer arithmetic with fractional precision
// Will be fully implemented in Phase 1
class FixedPoint {
public:
  constexpr FixedPoint() : value(0) {}
  constexpr explicit FixedPoint(i64 rawValue) : value(rawValue) {}

  // Construction from integer
  static constexpr FixedPoint fromInt(int x) {
    return FixedPoint(static_cast<i64>(x) << kFixedPointFractionalBits);
  }

  // Construction from double (for initialization/testing)
  static constexpr FixedPoint fromDouble(double x) {
    return FixedPoint(static_cast<i64>(x * kFixedPointScale));
  }

  // Conversion to integer (truncates)
  constexpr int toInt() const {
    return static_cast<int>(value >> kFixedPointFractionalBits);
  }

  // Conversion to double (for debugging/output)
  constexpr double toDouble() const {
    return static_cast<double>(value) / kFixedPointScale;
  }

  // Raw value access
  constexpr i64 raw() const { return value; }

  // Arithmetic operators (stub - to be implemented in Phase 1)
  constexpr FixedPoint operator+(FixedPoint other) const {
    return FixedPoint(value + other.value);
  }

  constexpr FixedPoint operator-(FixedPoint other) const {
    return FixedPoint(value - other.value);
  }

  // Comparison operators
  constexpr bool operator==(FixedPoint other) const { return value == other.value; }
  constexpr bool operator!=(FixedPoint other) const { return value != other.value; }
  constexpr bool operator<(FixedPoint other) const { return value < other.value; }
  constexpr bool operator<=(FixedPoint other) const { return value <= other.value; }
  constexpr bool operator>(FixedPoint other) const { return value > other.value; }
  constexpr bool operator>=(FixedPoint other) const { return value >= other.value; }

private:
  i64 value;
};

} // namespace freerouting

#endif // FREEROUTING_CORE_FIXEDPOINT_H
