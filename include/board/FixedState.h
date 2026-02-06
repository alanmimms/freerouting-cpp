#ifndef FREEROUTING_BOARD_FIXEDSTATE_H
#define FREEROUTING_BOARD_FIXEDSTATE_H

namespace freerouting {

// Fixed states of board items
// Sorted from weakest to strongest - stronger fixed states override weaker ones
enum class FixedState {
  // Item is not fixed - can be moved or deleted freely
  NotFixed = 0,

  // Item can be pushed (shoved) during routing but not deleted
  ShoveFix = 1,

  // Item is fixed by the user - cannot be moved or pushed
  UserFixed = 2,

  // Item is fixed by the system - strongest protection
  SystemFixed = 3
};

// Check if an item with a given fixed state can be moved
constexpr bool canMove(FixedState state) {
  return state == FixedState::NotFixed;
}

// Check if an item with a given fixed state can be pushed/shoved
constexpr bool canShove(FixedState state) {
  return state == FixedState::NotFixed || state == FixedState::ShoveFix;
}

// Check if an item with a given fixed state is user-fixed or stronger
constexpr bool isUserFixed(FixedState state) {
  return state >= FixedState::UserFixed;
}

// Check if an item with a given fixed state is system-fixed
constexpr bool isSystemFixed(FixedState state) {
  return state == FixedState::SystemFixed;
}

// Get stronger of two fixed states
constexpr FixedState maxFixedState(FixedState a, FixedState b) {
  return (a > b) ? a : b;
}

} // namespace freerouting

#endif // FREEROUTING_BOARD_FIXEDSTATE_H
