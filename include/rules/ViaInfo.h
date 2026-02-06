#ifndef FREEROUTING_RULES_VIAINFO_H
#define FREEROUTING_RULES_VIAINFO_H

#include "core/Padstack.h"
#include <string>

namespace freerouting {

// Forward declaration
class BoardRules;

// Information about a via padstack, clearance class, and routing properties
// Used in interactive and automatic routing to define via characteristics
class ViaInfo {
public:
  // Constructor
  ViaInfo(std::string viaName, const Padstack* padstackPtr,
          int clearanceClass, bool attachSmd)
    : name_(std::move(viaName)),
      padstack_(padstackPtr),
      clearanceClass_(clearanceClass),
      attachSmdAllowed_(attachSmd) {}

  // Default constructor
  ViaInfo()
    : name_(""),
      padstack_(nullptr),
      clearanceClass_(0),
      attachSmdAllowed_(false) {}

  // Getters
  const std::string& getName() const { return name_; }
  const Padstack* getPadstack() const { return padstack_; }
  int getClearanceClass() const { return clearanceClass_; }
  bool isAttachSmdAllowed() const { return attachSmdAllowed_; }

  // Setters
  void setName(const std::string& name) { name_ = name; }
  void setPadstack(const Padstack* padstack) { padstack_ = padstack; }
  void setClearanceClass(int clearanceClass) { clearanceClass_ = clearanceClass; }
  void setAttachSmdAllowed(bool allowed) { attachSmdAllowed_ = allowed; }

  // Comparison by name (for sorting)
  bool operator<(const ViaInfo& other) const {
    return name_ < other.name_;
  }

  bool operator==(const ViaInfo& other) const {
    return padstack_ == other.padstack_ &&
           clearanceClass_ == other.clearanceClass_;
  }

private:
  std::string name_;            // Via name
  const Padstack* padstack_;    // Padstack definition (non-owning pointer)
  int clearanceClass_;          // Clearance class index
  bool attachSmdAllowed_;       // Allow attachment to SMD pads
};

} // namespace freerouting

#endif // FREEROUTING_RULES_VIAINFO_H
