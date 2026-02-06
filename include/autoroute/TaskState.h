#ifndef FREEROUTING_AUTOROUTE_TASKSTATE_H
#define FREEROUTING_AUTOROUTE_TASKSTATE_H

namespace freerouting {

// State of a routing or optimization task
enum class TaskState {
  Idle,
  Started,
  Running,
  Finished,
  Cancelled,
  TimedOut
};

} // namespace freerouting

#endif // FREEROUTING_AUTOROUTE_TASKSTATE_H
