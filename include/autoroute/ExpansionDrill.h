#ifndef FREEROUTING_AUTOROUTE_EXPANSIONDRILL_H
#define FREEROUTING_AUTOROUTE_EXPANSIONDRILL_H

#include "autoroute/ExpandableObject.h"
#include "autoroute/CompleteExpansionRoom.h"
#include "autoroute/MazeSearchElement.h"
#include "geometry/Shape.h"
#include "geometry/Vector2.h"
#include <vector>

namespace freerouting {

// Forward declarations
class AutorouteEngine;

// Layer change expansion object in the maze search algorithm
// Represents a potential via location
class ExpansionDrill : public ExpandableObject {
public:
  // The location where the drill is checked
  IntPoint location;

  // The first layer of the drill
  int firstLayer;

  // The last layer of the drill
  int lastLayer;

  // Array of dimension lastLayer - firstLayer + 1
  std::vector<CompleteExpansionRoom*> roomArray;

  // Creates a new ExpansionDrill
  ExpansionDrill(const Shape* drillShape, IntPoint drillLocation,
                 int firstLyr, int lastLyr);

  ~ExpansionDrill() override = default;

  // Looks for the expansion room of this drill on each layer
  // Creates a CompleteFreeSpaceExpansionRoom if no expansion room is found
  // Returns false if that was not possible because of an obstacle
  bool calculateExpansionRooms(AutorouteEngine* autorouteEngine);

  // Get the shape of the drill
  const Shape* getShape() const override {
    return shape;
  }

  // Get the dimension (always 2 for drills)
  int getDimension() const override {
    return 2;
  }

  // Other room (always returns nullptr for drills)
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

  // Reset for the next connection
  void reset() override;

private:
  const Shape* shape;
  std::vector<MazeSearchElement> mazeSearchInfoArray;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_EXPANSIONDRILL_H
