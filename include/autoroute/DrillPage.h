#ifndef FREEROUTING_AUTOROUTE_DRILLPAGE_H
#define FREEROUTING_AUTOROUTE_DRILLPAGE_H

#include "autoroute/ExpandableObject.h"
#include "autoroute/ExpansionDrill.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/IntBox.h"
#include <vector>

namespace freerouting {

// Forward declarations
class RoutingBoard;
class AutorouteEngine;

// A drill page is inserted between an expansion room and the drill to expand
// in order to prevent performance problems with rooms with big shapes containing many drills
class DrillPage : public ExpandableObject {
public:
  // The shape of the page
  IntBox shape;

  // Creates a new DrillPage
  DrillPage(IntBox pageShape, RoutingBoard* routingBoard);

  ~DrillPage() override = default;

  // Returns the drills on this page
  // If attachSmd is true, drilling to smd pins is allowed
  std::vector<ExpansionDrill*> getDrills(AutorouteEngine* autorouteEngine, bool attachSmd);

  // Get the shape of this page
  const Shape* getShape() const override {
    // Note: IntBox needs to implement Shape interface or we need conversion
    return reinterpret_cast<const Shape*>(&shape);
  }

  // Get the dimension (always 2 for pages)
  int getDimension() const override {
    return 2;
  }

  // Other room (always returns nullptr for drill pages)
  CompleteExpansionRoom* otherRoom(CompleteExpansionRoom* room) override {
    return nullptr;
  }

  // MazeSearchElement access
  int mazeSearchElementCount() const override {
    return static_cast<int>(mazeSearchInfoArray.size());
  }

  MazeSearchElement& getMazeSearchElement(int no) override {
    return mazeSearchInfoArray[no];
  }

  const MazeSearchElement& getMazeSearchElement(int no) const override {
    return mazeSearchInfoArray[no];
  }

  // Resets all drills of this page for autorouting the next connection
  void reset() override;

  // Invalidates the drills of this page so they are recalculated at the next call of getDrills()
  void invalidate() {
    drills.clear();
    netNo = -1;
  }

private:
  RoutingBoard* board;
  std::vector<MazeSearchElement> mazeSearchInfoArray;

  // The list of expansion drills on this page (null if not yet calculated)
  std::vector<ExpansionDrill*> drills;

  // The number of the net for which the drills are calculated
  int netNo;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_DRILLPAGE_H
