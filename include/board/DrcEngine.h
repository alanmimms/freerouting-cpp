#ifndef FREEROUTING_BOARD_DRCENGINE_H
#define FREEROUTING_BOARD_DRCENGINE_H

#include "board/DrcViolation.h"
#include "board/BasicBoard.h"
#include <vector>
#include <memory>

namespace freerouting {

// Design Rule Checker engine
// Validates board against design rules and identifies violations
class DrcEngine {
public:
  // Constructor
  explicit DrcEngine(BasicBoard* board)
    : board_(board) {}

  // Run all DRC checks
  std::vector<DrcViolation> checkAll();

  // Individual check methods
  std::vector<DrcViolation> checkClearances();
  std::vector<DrcViolation> checkTraceWidths();
  std::vector<DrcViolation> checkRuleAreas();
  std::vector<DrcViolation> checkNetConflicts();

  // Get statistics
  int getErrorCount() const { return errorCount_; }
  int getWarningCount() const { return warningCount_; }

  // Clear statistics
  void clearStatistics() {
    errorCount_ = 0;
    warningCount_ = 0;
  }

private:
  BasicBoard* board_;
  int errorCount_ = 0;
  int warningCount_ = 0;

  // Helper methods

  // Check clearance between two items
  bool checkClearanceBetweenItems(Item* item1, Item* item2,
                                   std::vector<DrcViolation>& violations);

  // Check if an item violates any rule areas
  bool checkItemAgainstRuleAreas(Item* item,
                                  std::vector<DrcViolation>& violations);

  // Get required clearance between two items
  int getRequiredClearance(Item* item1, Item* item2) const;

  // Calculate actual clearance between two items (using bounding boxes)
  int calculateClearance(Item* item1, Item* item2) const;

  // Update statistics from violations
  void updateStatistics(const std::vector<DrcViolation>& violations) {
    for (const auto& v : violations) {
      if (v.getSeverity() == DrcViolation::Severity::Error) {
        ++errorCount_;
      } else {
        ++warningCount_;
      }
    }
  }
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_DRCENGINE_H
