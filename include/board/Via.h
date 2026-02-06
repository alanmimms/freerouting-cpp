#ifndef FREEROUTING_BOARD_VIA_H
#define FREEROUTING_BOARD_VIA_H

#include "DrillItem.h"

namespace freerouting {

// Via on a PCB board
// Vias connect traces between different layers
class Via : public DrillItem {
public:
  // Create a new via
  Via(IntPoint center, const Padstack* padstack,
      const std::vector<int>& nets, int clearanceClass, int id,
      FixedState fixedState, bool attachAllowed, BasicBoard* board)
    : DrillItem(center, nets, clearanceClass, id, 0 /* no component */,
                fixedState, board),
      padstack_(padstack),
      attachAllowed_(attachAllowed) {}

  // Get padstack
  const Padstack* getPadstack() const override { return padstack_; }

  // Set padstack
  void setPadstack(const Padstack* padstack) { padstack_ = padstack; }

  // Check if SMD attachment is allowed
  bool isAttachAllowed() const { return attachAllowed_; }

  // Vias are routable if not user-fixed and have nets
  bool isRoutable() const override {
    return !isUserFixed() && netCount() > 0;
  }

  // Check if this via is an obstacle for another item
  bool isObstacle(const Item& other) const override {
    // Same via is not an obstacle
    if (&other == this) return false;

    // Vias of the same net are typically not obstacles
    // (though there may be exceptions for spacing rules)
    if (sharesNet(other)) return false;

    // Different net = obstacle
    return true;
  }

  // Create a copy with new ID
  Item* copy(int newId) const override {
    return new Via(getCenter(), padstack_, getNets(), getClearanceClass(),
                   newId, getFixedState(), attachAllowed_, getBoard());
  }

private:
  const Padstack* padstack_;   // Padstack defining shape
  bool attachAllowed_;         // Allow SMD attachment
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_VIA_H
