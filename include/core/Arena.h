#ifndef FREEROUTING_CORE_ARENA_H
#define FREEROUTING_CORE_ARENA_H

#include "Types.h"
#include <memory>
#include <cstdlib>

namespace freerouting {

// Thread-local arena allocator for routing passes
// Provides fast, bulk allocation with stack-based lifetime
// Full implementation will be completed in Phase 1
class Arena {
public:
  // Default arena size: 64MB per thread
  static constexpr size_t kDefaultArenaSize = 64 * 1024 * 1024;

  explicit Arena(size_t size = kDefaultArenaSize)
    : buffer(nullptr), capacity(size), used(0) {
    // Stub: will allocate actual memory in Phase 1
  }

  ~Arena() {
    // Stub: will free memory in Phase 1
  }

  // Allocate memory for n objects of type T
  template<typename T>
  T* alloc(size_t n = 1) {
    // Stub: temporarily use standard allocation
    // Phase 1 will implement proper arena allocation
    return static_cast<T*>(std::malloc(sizeof(T) * n));
  }

  // Reset arena to beginning (invalidates all previous allocations)
  void reset() {
    used = 0;
  }

  // Get current memory usage
  size_t bytesUsed() const {
    return used;
  }

  // Get arena capacity
  size_t bytesCapacity() const {
    return capacity;
  }

  // Non-copyable
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

private:
  char* buffer;
  size_t capacity;
  size_t used;
};

// Thread-local arena for the current routing pass
// Each thread gets its own arena to avoid synchronization
inline thread_local Arena* threadArena = nullptr;

// RAII helper to set thread-local arena
class ArenaScope {
public:
  explicit ArenaScope(Arena& arena) : previous(threadArena) {
    threadArena = &arena;
  }

  ~ArenaScope() {
    threadArena = previous;
  }

  // Non-copyable
  ArenaScope(const ArenaScope&) = delete;
  ArenaScope& operator=(const ArenaScope&) = delete;

private:
  Arena* previous;
};

} // namespace freerouting

#endif // FREEROUTING_CORE_ARENA_H
