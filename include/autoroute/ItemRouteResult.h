#ifndef FREEROUTING_AUTOROUTE_ITEMROUTERESULT_H
#define FREEROUTING_AUTOROUTE_ITEMROUTERESULT_H

namespace freerouting {

// Tracks the result of routing/optimizing a single item
// Used to compare before/after metrics
class ItemRouteResult {
public:
  // Constructor for failed/skipped items
  explicit ItemRouteResult(int itemId)
    : itemId(itemId),
      viaCountBefore(0),
      viaCountAfter(0),
      traceLengthBefore(0.0),
      traceLengthAfter(0.0),
      incompleteCountBefore(0),
      incompleteCountAfter(1),
      improved(false),
      improvementPercentage(0.0f) {}

  // Constructor with full metrics
  ItemRouteResult(int itemId, int viasBefore, int viasAfter,
                  double lengthBefore, double lengthAfter,
                  int incompleteBefore, int incompleteAfter)
    : itemId(itemId),
      viaCountBefore(viasBefore),
      viaCountAfter(viasAfter),
      traceLengthBefore(lengthBefore),
      traceLengthAfter(lengthAfter),
      incompleteCountBefore(incompleteBefore),
      incompleteCountAfter(incompleteAfter) {

    // Determine if routing improved
    if (incompleteCountAfter < incompleteCountBefore) {
      improved = true;
    } else if (incompleteCountAfter > incompleteCountBefore) {
      improved = false;
    } else {  // Same incomplete count
      if (viaCountAfter < viaCountBefore) {
        improved = true;
      } else if (viaCountAfter > viaCountBefore) {
        improved = false;
      } else {  // Same via count
        improved = traceLengthAfter < traceLengthBefore;
      }
    }

    // Calculate improvement percentage
    if (viaCountBefore != 0 && traceLengthBefore != 0.0) {
      double viaRatio = static_cast<double>(viaCountAfter) / viaCountBefore;
      double lengthRatio = traceLengthAfter / traceLengthBefore;
      improvementPercentage = static_cast<float>(1.0 - (viaRatio + lengthRatio) / 2.0);
    } else {
      improvementPercentage = 0.0f;
    }
  }

  // Getters
  int getItemId() const { return itemId; }
  bool isImproved() const { return improved; }
  float getImprovementPercentage() const { return improvementPercentage; }
  int getViaCount() const { return viaCountAfter; }
  double getTraceLength() const { return traceLengthAfter; }
  int getIncompleteCount() const { return incompleteCountAfter; }
  int getViaCountReduced() const { return viaCountBefore - viaCountAfter; }
  double getLengthReduced() const { return traceLengthBefore - traceLengthAfter; }

  // Update improved flag (used after undo/restore)
  void updateImproved(bool value) { improved = value; }

  // Comparison operator for sorting (better results are "less")
  bool operator<(const ItemRouteResult& other) const {
    if (incompleteCountAfter != other.incompleteCountAfter) {
      return incompleteCountAfter < other.incompleteCountAfter;
    }
    if (viaCountAfter != other.viaCountAfter) {
      return viaCountAfter < other.viaCountAfter;
    }
    return traceLengthAfter < other.traceLengthAfter;
  }

private:
  int itemId;
  int viaCountBefore;
  int viaCountAfter;
  double traceLengthBefore;
  double traceLengthAfter;
  int incompleteCountBefore;
  int incompleteCountAfter;
  bool improved;
  float improvementPercentage;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_ITEMROUTERESULT_H
