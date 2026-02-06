#ifndef FREEROUTING_DATASTRUCTURES_TIMELIMIT_H
#define FREEROUTING_DATASTRUCTURES_TIMELIMIT_H

#include <chrono>

namespace freerouting {

// Manages time limits for operations
// Used to stop autorouting after a specified duration
class TimeLimit {
public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;
  using Duration = std::chrono::milliseconds;

  // Create unlimited time limit
  TimeLimit()
    : startTime(Clock::now()),
      limitMs(-1) {}

  // Create time limit with specified milliseconds
  explicit TimeLimit(int milliseconds)
    : startTime(Clock::now()),
      limitMs(milliseconds) {}

  // Check if time limit has been exceeded
  bool isExceeded() const {
    if (limitMs < 0) {
      return false; // No limit
    }
    auto elapsed = std::chrono::duration_cast<Duration>(
      Clock::now() - startTime
    );
    return elapsed.count() >= limitMs;
  }

  // Get elapsed time in milliseconds
  long getElapsedMs() const {
    auto elapsed = std::chrono::duration_cast<Duration>(
      Clock::now() - startTime
    );
    return elapsed.count();
  }

  // Get remaining time in milliseconds
  long getRemainingMs() const {
    if (limitMs < 0) {
      return -1; // Unlimited
    }
    long elapsed = getElapsedMs();
    return limitMs - elapsed;
  }

  // Reset the timer
  void reset() {
    startTime = Clock::now();
  }

  // Set a new time limit
  void setLimit(int milliseconds) {
    limitMs = milliseconds;
  }

private:
  TimePoint startTime;
  long limitMs; // -1 = no limit
};

} // namespace freerouting

#endif // FREEROUTING_DATASTRUCTURES_TIMELIMIT_H
