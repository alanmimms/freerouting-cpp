#ifndef FREEROUTING_BOARD_LAYER_H
#define FREEROUTING_BOARD_LAYER_H

#include <string_view>

namespace freerouting {

// Describes the structure of a board layer
// Layers can be signal layers (for routing) or non-signal (e.g., power/ground planes)
struct Layer {
  // Layer name (e.g., "F.Cu", "In1.Cu", "B.Cu")
  const char* name;

  // True if this is a signal layer that can be used for routing
  // False for power/ground planes or mechanical layers
  bool isSignal;

  // Constructor
  constexpr Layer(const char* layerName, bool signal)
    : name(layerName), isSignal(signal) {}

  // Default constructor for empty layer
  constexpr Layer() : name(""), isSignal(false) {}
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_LAYER_H
