#ifndef FREEROUTING_BOARD_ITEM_H
#define FREEROUTING_BOARD_ITEM_H

#include "FixedState.h"
#include "geometry/IntBox.h"
#include "geometry/Vector2.h"
#include "core/Types.h"
#include "autoroute/ItemAutorouteInfo.h"
#include <vector>
#include <algorithm>

namespace freerouting {

// Forward declarations
class BasicBoard;

// Base class for all items on a PCB board
// Items include traces, vias, pins, pads, and other board objects
//
// NOTE: This is a simplified Phase 4 version focusing on essential functionality
// Full shape/geometry integration will be added in later phases
class Item {
public:
  virtual ~Item() = default;

  // Get unique identification number
  int getId() const { return id_; }

  // Get the nets this item belongs to
  const std::vector<int>& getNets() const { return netNumbers_; }

  // Get number of nets
  int netCount() const { return static_cast<int>(netNumbers_.size()); }

  // Check if item belongs to a specific net
  bool containsNet(int netNumber) const {
    if (netNumber <= 0) return false;
    return std::find(netNumbers_.begin(), netNumbers_.end(), netNumber) != netNumbers_.end();
  }

  // Check if this item shares any net with another item
  bool sharesNet(const Item& other) const {
    for (int netNum : netNumbers_) {
      if (other.containsNet(netNum)) {
        return true;
      }
    }
    return false;
  }

  // Check if this item shares any net with a list of net numbers
  bool sharesNetWith(const std::vector<int>& otherNets) const {
    for (int netNum : netNumbers_) {
      for (int otherNet : otherNets) {
        if (netNum == otherNet) {
          return true;
        }
      }
    }
    return false;
  }

  // Get clearance class (index into clearance matrix)
  int getClearanceClass() const { return clearanceClass_; }

  // Set clearance class
  void setClearanceClass(int clearanceClass) { clearanceClass_ = clearanceClass; }

  // Get component number (0 if not part of a component)
  int getComponentNumber() const { return componentNumber_; }

  // Get fixed state
  FixedState getFixedState() const { return fixedState_; }

  // Set fixed state
  void setFixedState(FixedState state) { fixedState_ = state; }

  // Check if item is fixed by user or system
  bool isUserFixed() const { return freerouting::isUserFixed(fixedState_); }

  // Check if item can be moved
  bool canMove() const { return freerouting::canMove(fixedState_); }

  // Check if item can be shoved during routing
  bool canShove() const { return freerouting::canShove(fixedState_); }

  // Check if item is on the board (not deleted)
  bool isOnBoard() const { return onBoard_; }

  // Set on-board status
  void setOnBoard(bool onBoard) { onBoard_ = onBoard; }

  // Get board reference
  BasicBoard* getBoard() const { return board_; }

  // Get/set autoroute info
  ItemAutorouteInfo& getAutorouteInfo() { return autorouteInfo_; }
  const ItemAutorouteInfo& getAutorouteInfo() const { return autorouteInfo_; }
  void clearAutorouteInfo() { autorouteInfo_.clear(); }

  // Abstract methods to be implemented by derived classes

  // Check if this item is an obstacle for another item
  virtual bool isObstacle(const Item& other) const = 0;

  // Check if this item is an obstacle for a trace on a specific net
  virtual bool isTraceObstacle(int netNumber) const {
    return !containsNet(netNumber);
  }

  // Check if this item can be routed (is moveable routing item)
  virtual bool isRoutable() const {
    return !isUserFixed() && netCount() > 0;
  }

  // Get bounding box for this item
  virtual IntBox getBoundingBox() const = 0;

  // Get layer range (first layer and last layer)
  virtual int firstLayer() const = 0;
  virtual int lastLayer() const = 0;

  // Get number of layers this item spans
  int layerCount() const {
    return lastLayer() - firstLayer() + 1;
  }

  // Check if item is on a specific layer
  bool isOnLayer(int layer) const {
    return layer >= firstLayer() && layer <= lastLayer();
  }

  // Create a copy of this item with a new ID
  virtual Item* copy(int newId) const = 0;

  // Get the tree shape for spatial indexing
  // TODO: Implement when ShapeSearchTree is ported
  virtual const class Shape* getTreeShape(class ShapeSearchTree* tree, int index) const {
    (void)tree; (void)index;
    return nullptr;
  }

  // Get the layer of a specific shape index
  // TODO: Implement in derived classes that have multiple shapes per item
  virtual int shapeLayer(int index) const {
    (void)index;
    return firstLayer();
  }

  // Comparison for sorting by ID
  bool operator<(const Item& other) const {
    return id_ < other.id_;
  }

  bool operator==(const Item& other) const {
    return id_ == other.id_;
  }

protected:
  // Constructor for derived classes
  Item(const std::vector<int>& nets, int clearanceClass, int id,
       int componentNumber, FixedState fixedState, BasicBoard* board)
    : netNumbers_(nets),
      clearanceClass_(clearanceClass),
      id_(id),
      componentNumber_(componentNumber),
      fixedState_(fixedState),
      board_(board),
      onBoard_(false),
      autorouteInfo_() {}

  // Allow modification of net numbers by derived classes
  std::vector<int>& getMutableNets() { return netNumbers_; }

private:
  std::vector<int> netNumbers_;    // Nets this item belongs to
  int clearanceClass_;             // Index into clearance matrix
  int id_;                         // Unique identifier
  int componentNumber_;            // Component this belongs to (0 = none)
  FixedState fixedState_;          // Fixed/moveable state
  BasicBoard* board_;              // Board this item is on
  bool onBoard_;                   // True if inserted into board
  ItemAutorouteInfo autorouteInfo_; // Routing metadata
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_ITEM_H
