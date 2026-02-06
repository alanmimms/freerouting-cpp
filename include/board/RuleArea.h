#ifndef FREEROUTING_BOARD_RULEAREA_H
#define FREEROUTING_BOARD_RULEAREA_H

#include "board/Item.h"
#include "geometry/IntBox.h"
#include <vector>
#include <string>

namespace freerouting {

// Forward declarations
class Shape;

// Represents a rule area (keepout zone) on the board
// Rule areas restrict where traces, vias, and copper pours can be placed
// This addresses KiCad's rule areas that the Java freerouting ignores
class RuleArea : public Item {
public:
  // Types of restrictions a rule area can enforce
  enum class RestrictionType {
    Traces,       // Prohibit routing traces through area
    Vias,         // Prohibit via placement in area
    CopperPour,   // Prohibit copper pour in area
    All           // Prohibit all copper
  };

  // Create rule area with restrictions
  RuleArea(int idNo,
           int layerNo,
           const std::string& areaName,
           bool tracesAllowed,
           bool viasAllowed,
           bool copperPourAllowed,
           const std::vector<int>& affectedNets = {})
    : Item(affectedNets, 0, idNo, 0, FixedState::SystemFixed, nullptr),
      layer(layerNo),
      name(areaName),
      allowTraces(tracesAllowed),
      allowVias(viasAllowed),
      allowCopperPour(copperPourAllowed) {}

  // Get area name
  const std::string& getName() const { return name; }

  // Check if specific restriction type is prohibited
  bool isProhibited(RestrictionType type) const {
    switch (type) {
      case RestrictionType::Traces:
        return !allowTraces;
      case RestrictionType::Vias:
        return !allowVias;
      case RestrictionType::CopperPour:
        return !allowCopperPour;
      case RestrictionType::All:
        return !allowTraces && !allowVias && !allowCopperPour;
    }
    return false;
  }

  // Check if traces are allowed
  bool tracesAllowed() const { return allowTraces; }

  // Check if vias are allowed
  bool viasAllowed() const { return allowVias; }

  // Check if copper pour is allowed
  bool copperPourAllowed() const { return allowCopperPour; }

  // Check if this rule area affects a specific net
  // Empty affectedNets means it affects all nets
  bool affectsNet(int netNo) const {
    if (getNets().empty()) {
      return true;  // Affects all nets
    }
    return containsNet(netNo);
  }

  // Check if a point is inside this rule area
  // (Stub - requires actual shape geometry)
  bool contains(IntPoint point) const {
    (void)point;
    // TODO: Implement when shape geometry is available
    return false;
  }

  // Rule areas are obstacles based on restriction type
  bool isObstacle(const Item& other) const override {
    // Rule areas themselves aren't obstacles to items
    // (they're enforced differently during routing)
    (void)other;
    return false;
  }

  // Get bounding box
  IntBox getBoundingBox() const override {
    // TODO: Calculate from actual area shape
    return IntBox();
  }

  // Get first layer
  int firstLayer() const override {
    return layer;
  }

  // Get last layer
  int lastLayer() const override {
    return layer;
  }

  // Copy method
  Item* copy(int newId) const override {
    return new RuleArea(newId, layer, name,
                        allowTraces, allowVias, allowCopperPour,
                        getNets());
  }

  // Set restriction flags
  void setTracesAllowed(bool allowed) { allowTraces = allowed; }
  void setViasAllowed(bool allowed) { allowVias = allowed; }
  void setCopperPourAllowed(bool allowed) { allowCopperPour = allowed; }

private:
  int layer;
  std::string name;
  bool allowTraces;
  bool allowVias;
  bool allowCopperPour;
  // TODO: Add area shape when Shape hierarchy is complete
  // Shape* areaShape;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_RULEAREA_H
