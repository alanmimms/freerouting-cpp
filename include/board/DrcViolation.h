#ifndef FREEROUTING_BOARD_DRCVIOLATION_H
#define FREEROUTING_BOARD_DRCVIOLATION_H

#include "geometry/Vector2.h"
#include "geometry/IntBox.h"
#include <string>
#include <vector>

namespace freerouting {

// Forward declaration
class Item;

// Represents a single Design Rule Check violation
class DrcViolation {
public:
  // Types of DRC violations
  enum class Type {
    Clearance,        // Insufficient clearance between items
    TraceWidth,       // Trace width constraint violation
    RuleArea,         // Item violates a keepout/rule area
    NetConflict,      // Items with different nets touching
    UnconnectedNet,   // Net has unconnected pins
    ViaSize,          // Via size constraint violation
    DrillSize,        // Drill size constraint violation
    General           // Other/generic violation
  };

  // Severity levels
  enum class Severity {
    Error,    // Must be fixed
    Warning   // Should be reviewed
  };

  // Constructor
  DrcViolation(Type type, Severity severity, const std::string& message)
    : type_(type),
      severity_(severity),
      message_(message),
      layer_(0) {}

  // Getters
  Type getType() const { return type_; }
  Severity getSeverity() const { return severity_; }
  const std::string& getMessage() const { return message_; }
  int getLayer() const { return layer_; }
  IntPoint getLocation() const { return location_; }
  const std::vector<int>& getItemIds() const { return itemIds_; }

  // Setters
  void setLayer(int layer) { layer_ = layer; }
  void setLocation(IntPoint loc) { location_ = loc; }
  void addItemId(int itemId) { itemIds_.push_back(itemId); }

  // Formatted output
  std::string toString() const {
    std::string result = "[";
    result += (severity_ == Severity::Error) ? "ERROR" : "WARNING";
    result += "] ";
    result += typeToString(type_);
    result += ": ";
    result += message_;
    return result;
  }

  // Type to string conversion
  static std::string typeToString(Type type) {
    switch (type) {
      case Type::Clearance: return "Clearance Violation";
      case Type::TraceWidth: return "Trace Width Violation";
      case Type::RuleArea: return "Rule Area Violation";
      case Type::NetConflict: return "Net Conflict";
      case Type::UnconnectedNet: return "Unconnected Net";
      case Type::ViaSize: return "Via Size Violation";
      case Type::DrillSize: return "Drill Size Violation";
      case Type::General: return "General Violation";
      default: return "Unknown Violation";
    }
  }

private:
  Type type_;
  Severity severity_;
  std::string message_;
  int layer_;
  IntPoint location_;
  std::vector<int> itemIds_;  // IDs of items involved in violation
};

} // namespace freerouting

#endif // FREEROUTING_BOARD_DRCVIOLATION_H
