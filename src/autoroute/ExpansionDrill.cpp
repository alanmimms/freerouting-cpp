#include "autoroute/ExpansionDrill.h"
#include "autoroute/AutorouteEngine.h"
#include "autoroute/IncompleteFreeSpaceExpansionRoom.h"
#include "autoroute/CompleteFreeSpaceExpansionRoom.h"

namespace freerouting {

ExpansionDrill::ExpansionDrill(const Shape* drillShape, IntPoint drillLocation,
                               int firstLyr, int lastLyr)
  : location(drillLocation),
    firstLayer(firstLyr),
    lastLayer(lastLyr),
    shape(drillShape) {

  int layerCount = lastLyr - firstLyr + 1;
  roomArray.resize(layerCount, nullptr);
  mazeSearchInfoArray.resize(layerCount);
}

bool ExpansionDrill::calculateExpansionRooms(AutorouteEngine* autorouteEngine) {
  // Create a point shape at the drill location
  // In Java: TileShape.get_instance(location)
  // For now, use a simplified approach - we need the location as a shape
  // This would typically create a degenerate shape containing just the point

  // TODO: Implement proper point shape when Shape class supports it
  // For now, we'll skip the overlapping objects search and directly create rooms

  for (int i = firstLayer; i <= lastLayer; i++) {
    CompleteExpansionRoom* foundRoom = nullptr;

    // In the full implementation, we would:
    // 1. Search for overlapping expansion rooms at this location/layer
    // 2. Use existing rooms if found
    // 3. Create new rooms if not found

    if (!foundRoom) {
      // Create a new expansion room on this layer
      // In Java: new IncompleteFreeSpaceExpansionRoom(null, i, searchShape)
      // Then complete it via autorouteEngine.complete_expansion_room()

      // Simplified for now - would need full room completion infrastructure
      // This is a placeholder that needs the full AutorouteEngine implementation

      // For now, return false to indicate we can't fully calculate yet
      // In the complete port, this would create and complete rooms
      return false;
    }

    roomArray[i - firstLayer] = foundRoom;
  }

  return true;
}

void ExpansionDrill::reset() {
  for (auto& mazeInfo : mazeSearchInfoArray) {
    mazeInfo.reset();
  }
}

} // namespace freerouting
