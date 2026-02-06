#ifndef FREEROUTING_AUTOROUTE_AUTOROUTEATTEMPTRESULT_H
#define FREEROUTING_AUTOROUTE_AUTOROUTEATTEMPTRESULT_H

#include "autoroute/AutorouteAttemptState.h"
#include <string>

namespace freerouting {

// Result of attempting to autoroute a connection
struct AutorouteAttemptResult {
  AutorouteAttemptState state;
  std::string details;

  // Constructor with state only
  explicit AutorouteAttemptResult(AutorouteAttemptState s)
    : state(s), details("") {}

  // Constructor with state and details
  AutorouteAttemptResult(AutorouteAttemptState s, const std::string& d)
    : state(s), details(d) {}
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_AUTOROUTEATTEMPTRESULT_H
