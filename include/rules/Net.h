#ifndef FREEROUTING_RULES_NET_H
#define FREEROUTING_RULES_NET_H

#include "NetClass.h"
#include <string>

namespace freerouting {

// Forward declaration
class Nets;

// Describes properties for an individual electrical net
// A net represents a collection of electrically connected items
// (pins, vias, traces) that must be routed together
class Net {
public:
  // Constructor
  Net(std::string netName, int subnetNumber, int netNumber,
      const NetClass* netClass, bool containsPlane = false)
    : name_(std::move(netName)),
      subnetNumber_(subnetNumber),
      netNumber_(netNumber),
      netClass_(netClass),
      containsPlane_(containsPlane) {}

  // Default constructor
  Net()
    : name_(""),
      subnetNumber_(1),
      netNumber_(0),
      netClass_(nullptr),
      containsPlane_(false) {}

  // Get name
  const std::string& getName() const {
    return name_;
  }

  // Get subnet number (used for divided nets with from-to rules)
  int getSubnetNumber() const {
    return subnetNumber_;
  }

  // Get unique net number
  int getNetNumber() const {
    return netNumber_;
  }

  // Get net class
  const NetClass* getNetClass() const {
    return netClass_;
  }

  // Set net class
  void setNetClass(const NetClass* netClass) {
    netClass_ = netClass;
  }

  // Check if net contains a power plane
  bool containsPlane() const {
    return containsPlane_;
  }

  // Set contains plane flag
  void setContainsPlane(bool contains) {
    containsPlane_ = contains;
  }

  // Comparison by name (for sorting)
  bool operator<(const Net& other) const {
    // Case-insensitive comparison
    for (size_t i = 0; i < name_.size() && i < other.name_.size(); i++) {
      char c1 = std::tolower(name_[i]);
      char c2 = std::tolower(other.name_[i]);
      if (c1 != c2) {
        return c1 < c2;
      }
    }
    return name_.size() < other.name_.size();
  }

  bool operator==(const Net& other) const {
    return netNumber_ == other.netNumber_;
  }

private:
  std::string name_;
  int subnetNumber_;     // Subnet number (normally 1)
  int netNumber_;        // Unique identifier
  const NetClass* netClass_;  // Non-owning pointer to net class
  bool containsPlane_;   // True if net has power plane
};

} // namespace freerouting

#endif // FREEROUTING_RULES_NET_H
