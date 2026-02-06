#ifndef FREEROUTING_AUTOROUTE_AUTOROUTEATTEMPTSTATE_H
#define FREEROUTING_AUTOROUTE_AUTOROUTEATTEMPTSTATE_H

namespace freerouting {

// The possible results of auto-routing a connection
enum class AutorouteAttemptState {
  Unknown,            // Unknown result
  Skipped,            // Item was skipped
  NoUnconnectedNets,  // Item has no unconnected nets
  ConnectedToPlane,   // Item is connected to a conduction plane
  AlreadyConnected,   // Item is already connected
  NoConnections,      // The item has no connections to nets
  Routed,             // Item was successfully routed
  Failed,             // Routing failed
  InsertError         // Error inserting item
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_AUTOROUTEATTEMPTSTATE_H
