#ifndef FREEROUTING_RULES_VIARULE_H
#define FREEROUTING_RULES_VIARULE_H

#include "ViaInfo.h"
#include "core/Padstack.h"
#include <vector>
#include <string>

namespace freerouting {

// Contains an array of vias used for routing
// Vias at the beginning of the array are preferred over later vias
// This allows prioritizing smaller/cheaper vias during autorouting
class ViaRule {
public:
  // Empty via rule constant
  static ViaRule empty() {
    return ViaRule("empty");
  }

  // Constructor with name
  explicit ViaRule(std::string ruleName)
    : name_(std::move(ruleName)) {}

  // Default constructor
  ViaRule() : name_("") {}

  // Get name
  const std::string& getName() const {
    return name_;
  }

  // Add via to end of rule (lower priority)
  void appendVia(const ViaInfo& via) {
    vias_.push_back(via);
  }

  // Remove via from rule
  // Returns true if via was found and removed
  bool removeVia(const ViaInfo& via) {
    for (auto it = vias_.begin(); it != vias_.end(); ++it) {
      if (*it == via) {
        vias_.erase(it);
        return true;
      }
    }
    return false;
  }

  // Get number of vias in rule
  int viaCount() const {
    return static_cast<int>(vias_.size());
  }

  // Get via by index (0 = highest priority)
  const ViaInfo& getVia(int index) const {
    FR_ASSERT(index >= 0 && index < viaCount());
    return vias_[index];
  }

  // Get via by index (mutable)
  ViaInfo& getVia(int index) {
    FR_ASSERT(index >= 0 && index < viaCount());
    return vias_[index];
  }

  // Check if rule contains specific via
  bool contains(const ViaInfo& viaInfo) const {
    for (const auto& via : vias_) {
      if (via == viaInfo) {
        return true;
      }
    }
    return false;
  }

  // Check if rule contains via with specific padstack
  bool containsPadstack(const Padstack* padstack) const {
    for (const auto& via : vias_) {
      if (via.getPadstack() == padstack) {
        return true;
      }
    }
    return false;
  }

  // Find via matching layer range
  // Returns nullptr if no matching via found
  const ViaInfo* getViaForLayerRange(int fromLayer, int toLayer) const {
    for (const auto& via : vias_) {
      const Padstack* padstack = via.getPadstack();
      if (padstack &&
          padstack->fromLayer() == fromLayer &&
          padstack->toLayer() == toLayer) {
        return &via;
      }
    }
    return nullptr;
  }

  // Swap positions of two vias in the rule (for priority adjustment)
  // Returns false if either via not found
  bool swap(const ViaInfo& via1, const ViaInfo& via2) {
    int index1 = -1;
    int index2 = -1;

    for (int i = 0; i < viaCount(); i++) {
      if (vias_[i] == via1) index1 = i;
      if (vias_[i] == via2) index2 = i;
    }

    if (index1 < 0 || index2 < 0) {
      return false;
    }

    if (index1 != index2) {
      std::swap(vias_[index1], vias_[index2]);
    }

    return true;
  }

  // Iterator support
  auto begin() const { return vias_.begin(); }
  auto end() const { return vias_.end(); }
  auto begin() { return vias_.begin(); }
  auto end() { return vias_.end(); }

private:
  std::string name_;
  std::vector<ViaInfo> vias_;
};

} // namespace freerouting

#endif // FREEROUTING_RULES_VIARULE_H
