#ifndef FREEROUTING_DATASTRUCTURES_STOPPABLE_H
#define FREEROUTING_DATASTRUCTURES_STOPPABLE_H

namespace freerouting {

// Interface for operations that can be stopped/cancelled
// Used to allow cancellation of long-running autoroute operations
class Stoppable {
public:
  virtual ~Stoppable() = default;

  // Check if the operation should stop
  virtual bool isStopRequested() const = 0;

  // Request the operation to stop
  virtual void requestStop() = 0;

protected:
  Stoppable() = default;
};

// Simple implementation of Stoppable with a flag
class SimpleStoppable : public Stoppable {
public:
  SimpleStoppable() : stopRequested(false) {}

  bool isStopRequested() const override {
    return stopRequested;
  }

  void requestStop() override {
    stopRequested = true;
  }

  void reset() {
    stopRequested = false;
  }

private:
  bool stopRequested;
};

} // namespace freerouting

#endif // FREEROUTING_DATASTRUCTURES_STOPPABLE_H
