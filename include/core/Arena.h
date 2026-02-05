#ifndef FREEROUTING_CORE_ARENA_H
#define FREEROUTING_CORE_ARENA_H

#include "Types.h"
#include <memory>
#include <cstdlib>

namespace freerouting {

// Thread-local arena allocator for routing passes
// Provides fast, bulk allocation with stack-based lifetime
// Memory is never freed until arena is reset or destroyed
class Arena {
public:
  // Default arena size: 64MB per thread
  static constexpr size_t kDefaultArenaSize = 64 * 1024 * 1024;

  // Alignment for allocated memory (16 bytes for SIMD compatibility)
  static constexpr size_t kAlignment = 16;

  explicit Arena(size_t size = kDefaultArenaSize)
    : buffer(nullptr), capacity(size), used(0) {
    // Allocate aligned memory
    buffer = static_cast<char*>(std::aligned_alloc(kAlignment, capacity));
    FR_ASSERT_MSG(buffer != nullptr, "Arena allocation failed");
  }

  ~Arena() {
    if (buffer) {
      std::free(buffer);
      buffer = nullptr;
    }
  }

  // Allocate memory for n objects of type T
  // Memory is aligned to max(alignof(T), kAlignment)
  template<typename T>
  T* alloc(size_t n = 1) {
    constexpr size_t align = alignof(T) > kAlignment ? alignof(T) : kAlignment;
    size_t bytes = sizeof(T) * n;

    // Align current position
    size_t alignedUsed = (used + align - 1) & ~(align - 1);

    // Check if we have enough space
    if (alignedUsed + bytes > capacity) {
      FR_ASSERT_MSG(false, "Arena out of memory");
      return nullptr;
    }

    // Allocate and update position
    T* ptr = reinterpret_cast<T*>(buffer + alignedUsed);
    used = alignedUsed + bytes;

    return ptr;
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

  // Get remaining capacity
  size_t bytesRemaining() const {
    return capacity > used ? capacity - used : 0;
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
