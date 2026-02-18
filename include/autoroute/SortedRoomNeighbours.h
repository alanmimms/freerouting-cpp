#ifndef FREEROUTING_AUTOROUTE_SORTEDROOMNEIGHBOURS_H
#define FREEROUTING_AUTOROUTE_SORTEDROOMNEIGHBOURS_H

#include "autoroute/ExpansionRoom.h"
#include "autoroute/CompleteExpansionRoom.h"

namespace freerouting {

// Forward declarations
class AutorouteEngine;

/**
 * Helper class for calculating and sorting neighbour expansion rooms around a room.
 * Neighbours are sorted counterclockwise around the room's border.
 *
 * Translated from Java SortedRoomNeighbours.java
 */
class SortedRoomNeighbours {
public:
  /**
   * Main entry point - calculates doors for a room by finding and sorting neighbours.
   * Returns the completed room (may be the same as input or newly created).
   *
   * Java: SortedRoomNeighbours.calculate() lines 52-86
   */
  static CompleteExpansionRoom* calculate(ExpansionRoom* room, AutorouteEngine* autorouteEngine);

private:
  // Constructor is private - use static calculate() method
  SortedRoomNeighbours() = delete;
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_SORTEDROOMNEIGHBOURS_H
