#ifndef FREEROUTING_RULES_NETCLASS_H
#define FREEROUTING_RULES_NETCLASS_H

#include "ViaRule.h"
#include "ClearanceMatrix.h"
#include "board/LayerStructure.h"
#include <string>
#include <vector>
#include <algorithm>

namespace freerouting {

// Describes routing rules for individual nets or groups of nets
// Each net class defines:
// - Trace widths per layer
// - Clearance class for spacing rules
// - Via rules for layer transitions
// - Active routing layers
// - Behavioral flags (shove_fixed, pull_tight, etc.)
class NetClass {
public:
  // Constructor
  NetClass(std::string className, const LayerStructure& layerStructure,
           const ClearanceMatrix& clearanceMatrix, bool ignoredByAutorouter = false)
    : name_(std::move(className)),
      layerStructure_(&layerStructure),
      clearanceMatrix_(&clearanceMatrix),
      traceHalfWidths_(layerStructure.count(), 0),
      activeRoutingLayers_(layerStructure.count()),
      traceClearanceClass_(0),
      viaRule_(nullptr),
      shoveFix_(false),
      pullTight_(true),
      ignoreCyclesWithAreas_(false),
      minimumTraceLength_(0.0),
      maximumTraceLength_(0.0),
      isIgnoredByAutorouter_(ignoredByAutorouter) {

    // Initialize active routing layers based on signal layers
    for (int i = 0; i < layerStructure.count(); i++) {
      activeRoutingLayers_[i] = layerStructure[i].isSignal;
    }
  }

  // Get name
  const std::string& getName() const {
    return name_;
  }

  // Set name
  void setName(const std::string& name) {
    name_ = name;
  }

  // Get layer count
  int layerCount() const {
    return static_cast<int>(traceHalfWidths_.size());
  }

  // Set trace half width on all layers
  void setTraceHalfWidth(int width) {
    std::fill(traceHalfWidths_.begin(), traceHalfWidths_.end(), width);
  }

  // Set trace half width on all inner layers
  void setTraceHalfWidthOnInner(int width) {
    for (int i = 1; i < layerCount() - 1; i++) {
      traceHalfWidths_[i] = width;
    }
  }

  // Set trace half width on specific layer
  void setTraceHalfWidth(int layer, int width) {
    FR_ASSERT(layer >= 0 && layer < layerCount());
    traceHalfWidths_[layer] = width;
  }

  // Get trace half width on specific layer
  int getTraceHalfWidth(int layer) const {
    if (layer < 0 || layer >= layerCount()) {
      return 0;
    }
    return traceHalfWidths_[layer];
  }

  // Get trace clearance class
  int getTraceClearanceClass() const {
    return traceClearanceClass_;
  }

  // Set trace clearance class
  void setTraceClearanceClass(int clearanceClass) {
    traceClearanceClass_ = clearanceClass;
  }

  // Get via rule
  const ViaRule* getViaRule() const {
    return viaRule_;
  }

  // Set via rule
  void setViaRule(const ViaRule* viaRule) {
    viaRule_ = viaRule;
  }

  // Check if traces/vias can be pushed (shoved)
  bool isShoveFix() const {
    return shoveFix_;
  }

  // Set shove fixed flag
  void setShoveFix(bool fixed) {
    shoveFix_ = fixed;
  }

  // Check if traces are pulled tight
  bool isPullTight() const {
    return pullTight_;
  }

  // Set pull tight flag
  void setPullTight(bool tight) {
    pullTight_ = tight;
  }

  // Check if cycle removal ignores conduction areas
  bool getIgnoreCyclesWithAreas() const {
    return ignoreCyclesWithAreas_;
  }

  // Set ignore cycles with areas flag
  void setIgnoreCyclesWithAreas(bool ignore) {
    ignoreCyclesWithAreas_ = ignore;
  }

  // Get minimum trace length (0 = no restriction)
  double getMinimumTraceLength() const {
    return minimumTraceLength_;
  }

  // Set minimum trace length
  void setMinimumTraceLength(double length) {
    minimumTraceLength_ = length;
  }

  // Get maximum trace length (0 = no restriction)
  double getMaximumTraceLength() const {
    return maximumTraceLength_;
  }

  // Set maximum trace length
  void setMaximumTraceLength(double length) {
    maximumTraceLength_ = length;
  }

  // Check if layer is active for routing
  bool isActiveRoutingLayer(int layer) const {
    if (layer < 0 || layer >= layerCount()) {
      return false;
    }
    return activeRoutingLayers_[layer];
  }

  // Set layer active/inactive for routing
  void setActiveRoutingLayer(int layer, bool active) {
    if (layer >= 0 && layer < layerCount()) {
      activeRoutingLayers_[layer] = active;
    }
  }

  // Set all layers active/inactive
  void setAllLayersActive(bool active) {
    std::fill(activeRoutingLayers_.begin(), activeRoutingLayers_.end(), active);
  }

  // Set all inner layers active/inactive
  void setAllInnerLayersActive(bool active) {
    for (int i = 1; i < layerCount() - 1; i++) {
      activeRoutingLayers_[i] = active;
    }
  }

  // Check if autorouter should ignore this net class
  bool isIgnoredByAutorouter() const {
    return isIgnoredByAutorouter_;
  }

  // Set ignored by autorouter flag
  void setIgnoredByAutorouter(bool ignored) {
    isIgnoredByAutorouter_ = ignored;
  }

  // Check if trace width varies by layer
  bool traceWidthIsLayerDependent() const {
    if (traceHalfWidths_.empty()) return false;

    int compareWidth = traceHalfWidths_[0];
    for (int i = 1; i < layerCount(); i++) {
      if ((*layerStructure_)[i].isSignal) {
        if (traceHalfWidths_[i] != compareWidth) {
          return true;
        }
      }
    }
    return false;
  }

  // Check if trace width varies on inner layers
  bool traceWidthIsInnerLayerDependent() const {
    if (layerCount() <= 3) {
      return false;
    }

    // Find first inner signal layer
    int firstInnerLayer = 1;
    while (firstInnerLayer < layerCount() - 1 &&
           !(*layerStructure_)[firstInnerLayer].isSignal) {
      ++firstInnerLayer;
    }

    if (firstInnerLayer >= layerCount() - 1) {
      return false;
    }

    int compareWidth = traceHalfWidths_[firstInnerLayer];
    for (int i = firstInnerLayer + 1; i < layerCount() - 1; i++) {
      if ((*layerStructure_)[i].isSignal) {
        if (traceHalfWidths_[i] != compareWidth) {
          return true;
        }
      }
    }
    return false;
  }

private:
  std::string name_;
  const LayerStructure* layerStructure_;
  const ClearanceMatrix* clearanceMatrix_;
  std::vector<int> traceHalfWidths_;        // Half-width per layer
  std::vector<bool> activeRoutingLayers_;   // Active layers for routing
  int traceClearanceClass_;
  const ViaRule* viaRule_;                  // Non-owning pointer
  bool shoveFix_;                           // Can traces be pushed?
  bool pullTight_;                          // Pull traces tight?
  bool ignoreCyclesWithAreas_;              // Ignore cycles with conduction areas?
  double minimumTraceLength_;
  double maximumTraceLength_;
  bool isIgnoredByAutorouter_;
};

} // namespace freerouting

#endif // FREEROUTING_RULES_NETCLASS_H
