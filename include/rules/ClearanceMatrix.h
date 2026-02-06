#ifndef FREEROUTING_RULES_CLEARANCEMATRIX_H
#define FREEROUTING_RULES_CLEARANCEMATRIX_H

#include "core/Types.h"
#include "board/LayerStructure.h"
#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <strings.h>

namespace freerouting {

// NxN matrix describing spacing restrictions between N clearance classes
// on a fixed set of layers
//
// Each clearance class has a name (e.g., "default", "power", "signal")
// The matrix stores the minimum spacing required between items of different classes
// Spacing values are always even integers (rounded up if odd)
class ClearanceMatrix {
public:
  // Safety margin added to clearance values for conservative checking
  static constexpr int kClearanceSafetyMargin = 16;

  // Create clearance matrix with specified number of classes and layer structure
  ClearanceMatrix(int classCount, const LayerStructure& layerStructure,
                  const std::vector<std::string>& classNames)
    : layerStructure_(&layerStructure),
      classCount_(std::max(classCount, 1)),
      rows_(classCount_),
      maxValueOnLayer_(layerStructure.count(), 0) {

    int layerCount = layerStructure.count();
    for (int i = 0; i < classCount_; i++) {
      rows_[i].name = (i < static_cast<int>(classNames.size()))
                      ? classNames[i]
                      : "class_" + std::to_string(i);
      rows_[i].columns.resize(classCount_);
      rows_[i].maxValue.resize(layerCount, 0);

      for (int j = 0; j < classCount_; j++) {
        rows_[i].columns[j].layerValues.resize(layerCount, 0);
      }
    }
  }

  // Create default clearance matrix with "null" and "default" classes
  static ClearanceMatrix createDefault(const LayerStructure& layerStructure,
                                       int defaultValue) {
    std::vector<std::string> names = {"null", "default"};
    ClearanceMatrix result(2, layerStructure, names);
    result.setDefaultValue(defaultValue);
    return result;
  }

  // Get clearance class number by name
  // Returns -1 if not found
  int getClassNumber(std::string_view name) const {
    for (int i = 0; i < classCount_; i++) {
      // Case-insensitive comparison
      if (strcasecmp(rows_[i].name.c_str(), name.data()) == 0) {
        return i;
      }
    }
    return -1;
  }

  // Get clearance class name by number
  const std::string& getClassName(int classNumber) const {
    FR_ASSERT(classNumber >= 0 && classNumber < classCount_);
    return rows_[classNumber].name;
  }

  // Get number of clearance classes
  int getClassCount() const {
    return classCount_;
  }

  // Get number of layers
  int getLayerCount() const {
    return layerStructure_->count();
  }

  // Set all clearance values (for classes >= 1) to specified value on all layers
  void setDefaultValue(int value) {
    for (int layer = 0; layer < layerStructure_->count(); layer++) {
      setDefaultValue(layer, value);
    }
  }

  // Set all clearance values (for classes >= 1) to specified value on one layer
  void setDefaultValue(int layer, int value) {
    for (int i = 1; i < classCount_; i++) {
      for (int j = 1; j < classCount_; j++) {
        setValue(i, j, layer, value);
      }
    }
  }

  // Set clearance value between two classes on all layers
  void setValue(int classI, int classJ, int value) {
    for (int layer = 0; layer < layerStructure_->count(); layer++) {
      setValue(classI, classJ, layer, value);
    }
  }

  // Set clearance value between two classes on inner layers only
  void setInnerValue(int classI, int classJ, int value) {
    for (int layer = 1; layer < layerStructure_->count() - 1; layer++) {
      setValue(classI, classJ, layer, value);
    }
  }

  // Set clearance value between two classes on specific layer
  // Values are forced to be positive and even (rounded up if odd)
  void setValue(int classI, int classJ, int layer, int value) {
    FR_ASSERT(classI >= 0 && classI < classCount_);
    FR_ASSERT(classJ >= 0 && classJ < classCount_);
    FR_ASSERT(layer >= 0 && layer < layerStructure_->count());

    // Ensure value is positive and even
    int evenValue = std::max(value, 0);
    if (evenValue % 2 != 0) {
      if (evenValue == INT32_MAX) {
        evenValue--;
      } else {
        evenValue++;
      }
    }

    rows_[classJ].columns[classI].layerValues[layer] = evenValue;
    rows_[classJ].maxValue[layer] = std::max(rows_[classJ].maxValue[layer], evenValue);
    maxValueOnLayer_[layer] = std::max(maxValueOnLayer_[layer], evenValue);
  }

  // Get clearance value between two classes on specific layer
  // Returns 0 if indices out of range
  int getValue(int classI, int classJ, int layer, bool addSafetyMargin = false) const {
    if (classI < 0 || classI >= classCount_ ||
        classJ < 0 || classJ >= classCount_ ||
        layer < 0 || layer >= layerStructure_->count()) {
      return 0;
    }

    int value = rows_[classJ].columns[classI].layerValues[layer];
    return addSafetyMargin ? value + kClearanceSafetyMargin : value;
  }

  // Get maximum clearance value for a class on a layer
  // (maximum spacing this class needs from any other class)
  int maxValue(int classNumber, int layer) const {
    int cl = std::clamp(classNumber, 0, classCount_ - 1);
    int ly = std::clamp(layer, 0, layerStructure_->count() - 1);
    return rows_[cl].maxValue[ly];
  }

  // Get maximum clearance value on a layer across all classes
  int maxValue(int layer) const {
    int ly = std::clamp(layer, 0, layerStructure_->count() - 1);
    return maxValueOnLayer_[ly];
  }

  // Check if clearance values are layer-dependent for a class pair
  bool isLayerDependent(int classI, int classJ) const {
    if (classI < 0 || classI >= classCount_ ||
        classJ < 0 || classJ >= classCount_) {
      return false;
    }

    const auto& values = rows_[classJ].columns[classI].layerValues;
    int compareValue = values[0];
    for (int layer = 1; layer < layerStructure_->count(); layer++) {
      if (values[layer] != compareValue) {
        return true;
      }
    }
    return false;
  }

  // Check if clearance values are layer-dependent on inner layers only
  bool isInnerLayerDependent(int classI, int classJ) const {
    if (classI < 0 || classI >= classCount_ ||
        classJ < 0 || classJ >= classCount_) {
      return false;
    }

    if (layerStructure_->count() <= 2) {
      return false;  // No inner layers
    }

    const auto& values = rows_[classJ].columns[classI].layerValues;
    int compareValue = values[1];
    for (int layer = 2; layer < layerStructure_->count() - 1; layer++) {
      if (values[layer] != compareValue) {
        return true;
      }
    }
    return false;
  }

  // Get clearance compensation value for a class on a layer
  // This is half the self-clearance (used for offsetting shapes)
  int clearanceCompensation(int classNumber, int layer) const {
    return (getValue(classNumber, classNumber, layer, false) + 1) / 2;
  }

  // Check if two classes have identical clearance values
  bool isEqual(int class1, int class2) const {
    if (class1 == class2) {
      return true;
    }
    if (class1 < 0 || class2 < 0 || class1 >= classCount_ || class2 >= classCount_) {
      return false;
    }

    const auto& row1 = rows_[class1];
    const auto& row2 = rows_[class2];

    for (int i = 1; i < classCount_; i++) {
      const auto& entry1 = row1.columns[i].layerValues;
      const auto& entry2 = row2.columns[i].layerValues;

      for (int layer = 0; layer < layerStructure_->count(); layer++) {
        if (entry1[layer] != entry2[layer]) {
          return false;
        }
      }
    }
    return true;
  }

private:
  // Single entry in the clearance matrix
  struct MatrixEntry {
    std::vector<int> layerValues;  // Clearance value per layer
  };

  // Row in the clearance matrix
  struct Row {
    std::string name;                      // Clearance class name
    std::vector<MatrixEntry> columns;      // Clearance to each other class
    std::vector<int> maxValue;             // Max clearance on each layer
  };

  const LayerStructure* layerStructure_;
  int classCount_;
  std::vector<Row> rows_;
  std::vector<int> maxValueOnLayer_;  // Max clearance per layer across all classes
};

} // namespace freerouting

#endif // FREEROUTING_RULES_CLEARANCEMATRIX_H
