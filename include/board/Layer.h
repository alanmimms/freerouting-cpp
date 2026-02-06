#ifndef FREEROUTING_BOARD_LAYER_H
#define FREEROUTING_BOARD_LAYER_H

#include <string>
#include <string_view>

namespace freerouting {

// Describes the structure of a board layer
// Layers can be signal layers (for routing) or non-signal (e.g., power/ground planes)
struct Layer {
  // Layer name (e.g., "F.Cu", "In1.Cu", "B.Cu")
  // Using std::string to avoid lifetime issues
  std::string name;

  // True if this is a signal layer that can be used for routing
  // False for power/ground planes or mechanical layers
  bool isSignal;

  // Constructor
  Layer(std::string layerName, bool signal)
    : name(std::move(layerName)), isSignal(signal) {}

  // Constructor from C string
  Layer(const char* layerName, bool signal)
    : name(layerName), isSignal(signal) {}

  // Default constructor for empty layer
  Layer() : name(""), isSignal(false) {}
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_LAYER_H
