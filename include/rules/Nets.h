#ifndef FREEROUTING_RULES_NETS_H
#define FREEROUTING_RULES_NETS_H

#include "Net.h"
#include <vector>
#include <string_view>
#include <algorithm>

namespace freerouting {

// Collection of nets with lookup and management functions
// Provides indexed access and name-based lookup for nets
class Nets {
public:
  // Default constructor
  Nets() = default;

  // Add a net to the collection
  // Returns pointer to the added net
  Net* addNet(const Net& net) {
    nets_.push_back(net);
    return &nets_.back();
  }

  // Get net count
  int count() const {
    return static_cast<int>(nets_.size());
  }

  // Get net by index
  const Net& operator[](int index) const {
    FR_ASSERT(index >= 0 && index < count());
    return nets_[index];
  }

  // Get net by index (mutable)
  Net& operator[](int index) {
    FR_ASSERT(index >= 0 && index < count());
    return nets_[index];
  }

  // Find net by name
  // Returns nullptr if not found
  const Net* getNet(std::string_view name) const {
    for (const auto& net : nets_) {
      if (net.getName() == name) {
        return &net;
      }
    }
    return nullptr;
  }

  // Find net by name (mutable)
  Net* getNet(std::string_view name) {
    for (auto& net : nets_) {
      if (net.getName() == name) {
        return &net;
      }
    }
    return nullptr;
  }

  // Find net by net number
  // Returns nullptr if not found
  const Net* getNet(int netNumber) const {
    for (const auto& net : nets_) {
      if (net.getNetNumber() == netNumber) {
        return &net;
      }
    }
    return nullptr;
  }

  // Find net by net number (mutable)
  Net* getNet(int netNumber) {
    for (auto& net : nets_) {
      if (net.getNetNumber() == netNumber) {
        return &net;
      }
    }
    return nullptr;
  }

  // Get maximum net number currently in use
  int maxNetNumber() const {
    int maxNum = 0;
    for (const auto& net : nets_) {
      maxNum = std::max(maxNum, net.getNetNumber());
    }
    return maxNum;
  }

  // Sort nets by name (alphabetically)
  void sortByName() {
    std::sort(nets_.begin(), nets_.end());
  }

  // Iterator support
  auto begin() const { return nets_.begin(); }
  auto end() const { return nets_.end(); }
  auto begin() { return nets_.begin(); }
  auto end() { return nets_.end(); }

  // Clear all nets
  void clear() {
    nets_.clear();
  }

private:
  std::vector<Net> nets_;
};

} // namespace freerouting

#endif // FREEROUTING_RULES_NETS_H
