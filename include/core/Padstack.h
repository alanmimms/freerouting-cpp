#ifndef FREEROUTING_CORE_PADSTACK_H
#define FREEROUTING_CORE_PADSTACK_H

#include "core/Types.h"
#include <string>

namespace freerouting {

// Forward declaration
class Padstacks;

// Describes padstack masks for pins or vias located at the origin
// A padstack defines the copper shape on each layer for a pin or via
//
// NOTE: This is a simplified version for Phase 2 (Rules & Board Model)
// Full shape definitions will be added in later phases when we implement
// the geometry/shape system (ConvexShape, etc.)
class Padstack {
public:
  // Name of the padstack
  std::string name;

  // Unique identifier number
  int number;

  // True if vias of the same net are allowed to overlap with this padstack
  bool attachAllowed;

  // If false, layers are mirrored when placed on back side
  bool placedAbsolute;

  // Constructor
  Padstack(std::string padstackName, int padstackNumber,
           int fromLayerNum, int toLayerNum,
           bool attach = true, bool absolute = false)
    : name(std::move(padstackName)),
      number(padstackNumber),
      attachAllowed(attach),
      placedAbsolute(absolute),
      fromLayer_(fromLayerNum),
      toLayer_(toLayerNum) {}

  // Default constructor
  Padstack()
    : name(""),
      number(0),
      attachAllowed(false),
      placedAbsolute(false),
      fromLayer_(0),
      toLayer_(0) {}

  // Get first layer with non-null shape
  int fromLayer() const {
    return fromLayer_;
  }

  // Get last layer with non-null shape
  int toLayer() const {
    return toLayer_;
  }

  // Set layer range
  void setLayerRange(int from, int to) {
    fromLayer_ = from;
    toLayer_ = to;
  }

  // Comparison by name (for sorting)
  bool operator<(const Padstack& other) const {
    return name < other.name;
  }

  bool operator==(const Padstack& other) const {
    return number == other.number && name == other.name;
  }

private:
  int fromLayer_;  // First layer with shape
  int toLayer_;    // Last layer with shape

  // TODO Phase 3+: Add shape array when ConvexShape is implemented
  // std::vector<ConvexShape*> shapes_;
};

} // namespace freerouting

#endif // FREEROUTING_CORE_PADSTACK_H
