#ifndef FREEROUTING_BOARD_BASICBOARD_H
#define FREEROUTING_BOARD_BASICBOARD_H

#include "board/LayerStructure.h"
#include "rules/Nets.h"
#include "rules/ClearanceMatrix.h"
#include "board/Item.h"
#include "board/RuleArea.h"
#include <memory>
#include <vector>
#include <algorithm>

namespace freerouting {

// Basic board containing PCB items
// This is a simplified Phase 4 version focused on item management
// Full routing functionality (search trees, DRC, etc.) will be added in later phases
class BasicBoard {
public:
  // Constructor
  BasicBoard(const LayerStructure& layers, const ClearanceMatrix& clearanceMatrix)
    : layers_(layers),
      clearanceMatrix_(&clearanceMatrix),
      nextItemId_(1) {}

  // Get layer structure
  const LayerStructure& getLayers() const { return layers_; }

  // Get clearance matrix
  const ClearanceMatrix& getClearanceMatrix() const { return *clearanceMatrix_; }

  // Add an item to the board
  // Board takes ownership of the item
  void addItem(std::unique_ptr<Item> item) {
    if (item) {
      item->setOnBoard(true);
      items_.push_back(std::move(item));
    }
  }

  // Remove an item from the board by ID
  bool removeItem(int itemId) {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [itemId](const std::unique_ptr<Item>& item) {
                             return item->getId() == itemId;
                           });

    if (it != items_.end()) {
      (*it)->setOnBoard(false);
      items_.erase(it);
      return true;
    }
    return false;
  }

  // Get item by ID
  Item* getItem(int itemId) {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [itemId](const std::unique_ptr<Item>& item) {
                             return item->getId() == itemId;
                           });
    return (it != items_.end()) ? it->get() : nullptr;
  }

  const Item* getItem(int itemId) const {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [itemId](const std::unique_ptr<Item>& item) {
                             return item->getId() == itemId;
                           });
    return (it != items_.end()) ? it->get() : nullptr;
  }

  // Get all items
  const std::vector<std::unique_ptr<Item>>& getItems() const { return items_; }

  // Get item count
  size_t itemCount() const { return items_.size(); }

  // Get items by net
  std::vector<Item*> getItemsByNet(int netNumber) {
    std::vector<Item*> result;
    for (const auto& item : items_) {
      if (item->containsNet(netNumber)) {
        result.push_back(item.get());
      }
    }
    return result;
  }

  // Get items by layer
  std::vector<Item*> getItemsByLayer(int layer) {
    std::vector<Item*> result;
    for (const auto& item : items_) {
      if (item->isOnLayer(layer)) {
        result.push_back(item.get());
      }
    }
    return result;
  }

  // Generate a new unique item ID
  int generateItemId() {
    return nextItemId_++;
  }

  // Clear all items
  void clear() {
    items_.clear();
    nextItemId_ = 1;
  }

  // Get nets (optional - for integration with net management)
  void setNets(const Nets* nets) { nets_ = nets; }
  const Nets* getNets() const { return nets_; }

  // ========================================================================
  // Rule Area Support
  // ========================================================================

  // Add a rule area to the board
  void addRuleArea(std::unique_ptr<RuleArea> ruleArea) {
    if (ruleArea) {
      ruleArea->setOnBoard(true);
      ruleAreas_.push_back(std::move(ruleArea));
    }
  }

  // Get all rule areas
  const std::vector<std::unique_ptr<RuleArea>>& getRuleAreas() const {
    return ruleAreas_;
  }

  // Get rule areas affecting a specific layer
  std::vector<RuleArea*> getRuleAreasByLayer(int layer) {
    std::vector<RuleArea*> result;
    for (const auto& area : ruleAreas_) {
      if (area->isOnLayer(layer)) {
        result.push_back(area.get());
      }
    }
    return result;
  }

  // Check if a location is prohibited for a specific restriction type
  // Returns the first rule area that prohibits this location, or nullptr if allowed
  RuleArea* isLocationProhibited(IntPoint point,
                                 int layer,
                                 RuleArea::RestrictionType restrictionType,
                                 int netNo) const {
    for (const auto& area : ruleAreas_) {
      // Check if rule area is on this layer
      if (!area->isOnLayer(layer)) {
        continue;
      }

      // Check if rule area affects this net
      if (!area->affectsNet(netNo)) {
        continue;
      }

      // Check if this restriction type is prohibited
      if (!area->isProhibited(restrictionType)) {
        continue;
      }

      // Check if point is inside the rule area
      if (area->contains(point)) {
        return area.get();
      }
    }

    return nullptr;  // Location is allowed
  }

  // Convenience methods for specific restriction types
  bool isTraceProhibited(IntPoint point, int layer, int netNo) const {
    return isLocationProhibited(point, layer,
                                RuleArea::RestrictionType::Traces,
                                netNo) != nullptr;
  }

  bool isViaProhibited(IntPoint point, int layer, int netNo) const {
    return isLocationProhibited(point, layer,
                                RuleArea::RestrictionType::Vias,
                                netNo) != nullptr;
  }

  bool isCopperPourProhibited(IntPoint point, int layer, int netNo) const {
    return isLocationProhibited(point, layer,
                                RuleArea::RestrictionType::CopperPour,
                                netNo) != nullptr;
  }

  // Clear all rule areas
  void clearRuleAreas() {
    ruleAreas_.clear();
  }

private:
  LayerStructure layers_;
  const ClearanceMatrix* clearanceMatrix_;
  const Nets* nets_ = nullptr;
  std::vector<std::unique_ptr<Item>> items_;
  std::vector<std::unique_ptr<RuleArea>> ruleAreas_;
  int nextItemId_;
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_BASICBOARD_H
