#ifndef FREEROUTING_BOARD_ANGLERESTRICTION_H
#define FREEROUTING_BOARD_ANGLERESTRICTION_H

namespace freerouting {

// Angle restrictions for routing traces
// Determines which angles are permitted when creating trace segments
enum class AngleRestriction {
  // No angle restrictions - traces can be drawn at any angle
  None = 0,

  // 45-degree restriction - traces must be orthogonal or at 45° diagonals
  // Commonly used for high-speed designs
  FortyFiveDegree = 1,

  // 90-degree restriction - traces must be strictly orthogonal (horizontal/vertical)
  // Simpler routing, but may require more space
  NinetyDegree = 2
};

// Convert integer to AngleRestriction
constexpr AngleRestriction angleRestrictionFromInt(int value) {
  switch (value) {
    case 0: return AngleRestriction::None;
    case 1: return AngleRestriction::FortyFiveDegree;
    case 2: return AngleRestriction::NinetyDegree;
    default: return AngleRestriction::None;
  }
}

// Convert AngleRestriction to integer
constexpr int angleRestrictionToInt(AngleRestriction restriction) {
  return static_cast<int>(restriction);
}

} // namespace freerouting

#endif // FREEROUTING_BOARD_ANGLERESTRICTION_H
