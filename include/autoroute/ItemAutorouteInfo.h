#ifndef FREEROUTING_AUTOROUTE_ITEMAUTOROUTEINFO_H
#define FREEROUTING_AUTOROUTE_ITEMAUTOROUTEINFO_H

namespace freerouting {

// Forward declarations
class Connection;

// Per-item routing metadata
// Stores temporary data used during autorouting for each board item
// This is attached to Item objects during routing operations
class ItemAutorouteInfo {
public:
  ItemAutorouteInfo()
    : precalculatedConnection(nullptr),
      expansionDoor(nullptr),
      expansionDoorCount(0),
      isStart(false) {}

  // Get precalculated connection this item belongs to
  Connection* getPrecalculatedConnection() const {
    return precalculatedConnection;
  }

  // Set precalculated connection
  void setPrecalculatedConnection(Connection* connection) {
    precalculatedConnection = connection;
  }

  // Get expansion door (used in maze search)
  void* getExpansionDoor() const {
    return expansionDoor;
  }

  // Set expansion door
  void setExpansionDoor(void* door) {
    expansionDoor = door;
  }

  // Get expansion door count
  int getExpansionDoorCount() const {
    return expansionDoorCount;
  }

  // Set expansion door count
  void setExpansionDoorCount(int count) {
    expansionDoorCount = count;
  }

  // Increment expansion door count
  void incrementExpansionDoorCount() {
    ++expansionDoorCount;
  }

  // Check if this item is a start item (vs destination)
  bool isStartInfo() const {
    return isStart;
  }

  // Set whether this is a start item
  void setStartInfo(bool start) {
    isStart = start;
  }

  // Clear all routing info
  void clear() {
    precalculatedConnection = nullptr;
    expansionDoor = nullptr;
    expansionDoorCount = 0;
    isStart = false;
  }

private:
  Connection* precalculatedConnection;  // Cached connection for this item
  void* expansionDoor;                   // Expansion door for maze search
  int expansionDoorCount;                // Number of expansion doors
  bool isStart;                          // True if this is a start item
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_ITEMAUTOROUTEINFO_H
