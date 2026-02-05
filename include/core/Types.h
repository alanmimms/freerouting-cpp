#ifndef FREEROUTING_CORE_TYPES_H
#define FREEROUTING_CORE_TYPES_H

#include <cstdint>
#include <cassert>

namespace freerouting {

// Common type aliases
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

// Debug assertions - enabled in debug builds only
#ifdef NDEBUG
  #define FR_ASSERT(condition) ((void)0)
  #define FR_ASSERT_MSG(condition, message) ((void)0)
#else
  #define FR_ASSERT(condition) assert(condition)
  #define FR_ASSERT_MSG(condition, message) assert((condition) && (message))
#endif

// Force inline for performance-critical functions
#if defined(__GNUC__) || defined(__clang__)
  #define FR_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
  #define FR_FORCE_INLINE __forceinline
#else
  #define FR_FORCE_INLINE inline
#endif

} // namespace freerouting

#endif // FREEROUTING_CORE_TYPES_H
