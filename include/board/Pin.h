#ifndef FREEROUTING_BOARD_PIN_H
#define FREEROUTING_BOARD_PIN_H

#include "DrillItem.h"

namespace freerouting {

// Pin on a PCB component
// Pins are the connection points on components (ICs, resistors, etc.)
class Pin : public DrillItem {
public:
  // Create a new pin
  Pin(IntPoint center, int pinNumber, const Padstack* padstack,
      const std::vector<int>& nets, int clearanceClass, int id,
      int componentNumber, FixedState fixedState, BasicBoard* board)
    : DrillItem(center, nets, clearanceClass, id, componentNumber, fixedState, board),
      pinNumber_(pinNumber),
      padstack_(padstack) {}

  // Get pin number within component (0-based)
  int getPinNumber() const { return pinNumber_; }

  // Get padstack
  const Padstack* getPadstack() const override { return padstack_; }

  // Set padstack
  void setPadstack(const Padstack* padstack) { padstack_ = padstack; }

  // Pins are not routable (they are fixed endpoints)
  bool isRoutable() const override { return false; }

  // Check if this pin is an obstacle for another item
  bool isObstacle(const Item& other) const override {
    // Pins are obstacles for items on different nets
    // Pins of the same net can overlap with certain items
    if (&other == this) return false;
    if (sharesNet(other)) return false;
    return true;
  }

  // Create a copy with new ID
  Item* copy(int newId) const override {
    return new Pin(getCenter(), pinNumber_, padstack_, getNets(),
                   getClearanceClass(), newId, getComponentNumber(),
                   getFixedState(), getBoard());
  }

private:
  int pinNumber_;              // Pin number in component
  const Padstack* padstack_;   // Padstack defining shape
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_PIN_H
