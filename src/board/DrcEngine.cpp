#include "board/DrcEngine.h"
#include "board/Item.h"
#include "board/RuleArea.h"
#include "rules/ClearanceMatrix.h"
#include <algorithm>
#include <cmath>

namespace freerouting {

std::vector<DrcViolation> DrcEngine::checkAll() {
  clearStatistics();
  std::vector<DrcViolation> allViolations;

  // Run all checks
  auto clearanceViolations = checkClearances();
  auto traceWidthViolations = checkTraceWidths();
  auto ruleAreaViolations = checkRuleAreas();
  // Note: checkNetConflicts() is redundant with checkClearances() which now
  // skips same-net items, so all clearance violations are implicitly net conflicts

  // Combine all violations
  allViolations.insert(allViolations.end(),
                       clearanceViolations.begin(),
                       clearanceViolations.end());
  allViolations.insert(allViolations.end(),
                       traceWidthViolations.begin(),
                       traceWidthViolations.end());
  allViolations.insert(allViolations.end(),
                       ruleAreaViolations.begin(),
                       ruleAreaViolations.end());

  // Update statistics
  updateStatistics(allViolations);

  return allViolations;
}

std::vector<DrcViolation> DrcEngine::checkClearances() {
  std::vector<DrcViolation> violations;

  if (!board_) {
    return violations;
  }

  const auto& items = board_->getItems();

  // Check all pairs of items for clearance violations
  // This is O(n^2) but will be optimized with spatial indexing in Phase 12
  for (size_t i = 0; i < items.size(); ++i) {
    for (size_t j = i + 1; j < items.size(); ++j) {
      Item* item1 = items[i].get();
      Item* item2 = items[j].get();

      if (!item1 || !item2) {
        continue;
      }

      // Skip if items share a net (they're allowed to connect)
      if (item1->sharesNet(*item2)) {
        continue;
      }

      // Skip if items are on different layers with no overlap
      bool layerOverlap = false;
      for (int layer = item1->firstLayer(); layer <= item1->lastLayer(); ++layer) {
        if (item2->isOnLayer(layer)) {
          layerOverlap = true;
          break;
        }
      }

      if (!layerOverlap) {
        continue;
      }

      // Check clearance
      checkClearanceBetweenItems(item1, item2, violations);
    }
  }

  return violations;
}

std::vector<DrcViolation> DrcEngine::checkTraceWidths() {
  std::vector<DrcViolation> violations;

  if (!board_) {
    return violations;
  }

  // Trace width checking would require accessing trace-specific properties
  // For now, this is a placeholder that can be expanded when trace classes
  // have width constraints defined

  // Example logic (to be implemented when trace constraints are available):
  // for each trace item:
  //   get trace width
  //   get minimum allowed width for net class
  //   if (width < minWidth):
  //     create violation

  return violations;
}

std::vector<DrcViolation> DrcEngine::checkRuleAreas() {
  std::vector<DrcViolation> violations;

  if (!board_) {
    return violations;
  }

  const auto& items = board_->getItems();
  const auto& ruleAreas = board_->getRuleAreas();

  if (ruleAreas.empty()) {
    return violations;  // No rule areas to check
  }

  // Check each item against all rule areas
  for (const auto& item : items) {
    if (!item) {
      continue;
    }

    checkItemAgainstRuleAreas(item.get(), violations);
  }

  return violations;
}

std::vector<DrcViolation> DrcEngine::checkNetConflicts() {
  std::vector<DrcViolation> violations;

  if (!board_) {
    return violations;
  }

  const auto& items = board_->getItems();

  // Check for items with different nets that overlap/touch
  // This is similar to clearance checking but specifically for net conflicts
  for (size_t i = 0; i < items.size(); ++i) {
    for (size_t j = i + 1; j < items.size(); ++j) {
      Item* item1 = items[i].get();
      Item* item2 = items[j].get();

      if (!item1 || !item2) {
        continue;
      }

      // Get nets for both items
      auto nets1 = item1->getNets();
      auto nets2 = item2->getNets();

      if (nets1.empty() || nets2.empty()) {
        continue;  // Skip items without nets
      }

      // Check if items have different nets
      bool differentNets = true;
      for (int net1 : nets1) {
        for (int net2 : nets2) {
          if (net1 == net2) {
            differentNets = false;
            break;
          }
        }
        if (!differentNets) break;
      }

      if (!differentNets) {
        continue;  // Same net, no conflict
      }

      // Check if items overlap (clearance = 0)
      int clearance = calculateClearance(item1, item2);
      if (clearance <= 0) {
        DrcViolation violation(
          DrcViolation::Type::NetConflict,
          DrcViolation::Severity::Error,
          "Items with different nets are overlapping"
        );
        violation.addItemId(item1->getId());
        violation.addItemId(item2->getId());

        IntBox box1 = item1->getBoundingBox();
        violation.setLocation(box1.center().toInt());
        violation.setLayer(item1->firstLayer());

        violations.push_back(violation);
      }
    }
  }

  return violations;
}

bool DrcEngine::checkClearanceBetweenItems(Item* item1, Item* item2,
                                            std::vector<DrcViolation>& violations) {
  if (!item1 || !item2) {
    return true;  // No violation
  }

  // Get required clearance
  int requiredClearance = getRequiredClearance(item1, item2);

  // Calculate actual clearance
  int actualClearance = calculateClearance(item1, item2);

  // Check if violation exists
  if (actualClearance < requiredClearance) {
    DrcViolation violation(
      DrcViolation::Type::Clearance,
      DrcViolation::Severity::Error,
      "Clearance violation: required " +
      std::to_string(requiredClearance) +
      ", actual " +
      std::to_string(actualClearance)
    );

    violation.addItemId(item1->getId());
    violation.addItemId(item2->getId());

    // Set location to center of first item
    IntBox box1 = item1->getBoundingBox();
    violation.setLocation(box1.center().toInt());
    violation.setLayer(item1->firstLayer());

    violations.push_back(violation);
    return false;  // Violation found
  }

  return true;  // No violation
}

bool DrcEngine::checkItemAgainstRuleAreas(Item* item,
                                           std::vector<DrcViolation>& violations) {
  if (!item || !board_) {
    return true;  // No violation
  }

  const auto& ruleAreas = board_->getRuleAreas();
  IntBox itemBox = item->getBoundingBox();
  IntPoint itemCenter = itemBox.center().toInt();

  // Check each layer the item is on
  for (int layer = item->firstLayer(); layer <= item->lastLayer(); ++layer) {
    // Check against all rule areas on this layer
    for (const auto& area : ruleAreas) {
      if (!area->isOnLayer(layer)) {
        continue;
      }

      // Get item's nets
      auto nets = item->getNets();
      int netNo = nets.empty() ? 0 : nets[0];

      // Check if rule area affects this net
      if (!area->affectsNet(netNo)) {
        continue;
      }

      // Check if item violates the rule area
      // Note: This uses the stubbed contains() method
      // When geometry is implemented, this will properly check containment
      if (area->contains(itemCenter)) {
        // Determine what type of item this is for appropriate violation type
        // For now, we'll assume traces are prohibited
        if (area->isProhibited(RuleArea::RestrictionType::Traces)) {
          DrcViolation violation(
            DrcViolation::Type::RuleArea,
            DrcViolation::Severity::Error,
            "Item violates rule area '" + area->getName() + "'"
          );

          violation.addItemId(item->getId());
          violation.addItemId(area->getId());
          violation.setLocation(itemCenter);
          violation.setLayer(layer);

          violations.push_back(violation);
          return false;  // Violation found
        }
      }
    }
  }

  return true;  // No violation
}

int DrcEngine::getRequiredClearance(Item* item1, Item* item2) const {
  if (!board_ || !item1 || !item2) {
    return 0;
  }

  // Get clearance from the clearance matrix
  const auto& matrix = board_->getClearanceMatrix();

  // Use default clearance class (1 = "default")
  // Class 0 = "null" is not initialized by setDefaultValue()
  // In a full implementation, items would have their own clearance class IDs
  int clearanceClass1 = 1;
  int clearanceClass2 = 1;

  // Get clearance value for the first common layer
  int layer = item1->firstLayer();
  if (!item2->isOnLayer(layer)) {
    layer = item2->firstLayer();
  }

  return matrix.getValue(clearanceClass1, clearanceClass2, layer);
}

int DrcEngine::calculateClearance(Item* item1, Item* item2) const {
  if (!item1 || !item2) {
    return 0;
  }

  // Use bounding boxes for clearance calculation
  // This is a conservative approximation - actual shapes would be more accurate
  IntBox box1 = item1->getBoundingBox();
  IntBox box2 = item2->getBoundingBox();

  // Check if boxes overlap
  if (box1.intersects(box2)) {
    return 0;  // Overlapping or touching
  }

  // Calculate minimum distance between boxes
  int dx = 0;
  int dy = 0;

  // Horizontal separation
  if (box1.ur.x < box2.ll.x) {
    dx = box2.ll.x - box1.ur.x;
  } else if (box2.ur.x < box1.ll.x) {
    dx = box1.ll.x - box2.ur.x;
  }

  // Vertical separation
  if (box1.ur.y < box2.ll.y) {
    dy = box2.ll.y - box1.ur.y;
  } else if (box2.ur.y < box1.ll.y) {
    dy = box1.ll.y - box2.ur.y;
  }

  // Use Euclidean distance for more accurate clearance measurement
  // This is more accurate than Manhattan distance for diagonal separation
  if (dx > 0 && dy > 0) {
    // Boxes are diagonally separated - use Euclidean distance
    return static_cast<int>(std::sqrt(static_cast<double>(dx) * dx + static_cast<double>(dy) * dy));
  }

  // Boxes are aligned horizontally or vertically - use simple distance
  return dx + dy;
}

} // namespace freerouting
