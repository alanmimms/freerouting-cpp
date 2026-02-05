#include <catch2/catch_test_macros.hpp>
#include "core/Types.h"
#include "core/FixedPoint.h"
#include "core/Arena.h"

using namespace freerouting;

// Basic sanity tests for Phase 0 infrastructure

TEST_CASE("Types are defined correctly", "[types]") {
  SECTION("Integer types have correct sizes") {
    REQUIRE(sizeof(i8) == 1);
    REQUIRE(sizeof(i16) == 2);
    REQUIRE(sizeof(i32) == 4);
    REQUIRE(sizeof(i64) == 8);

    REQUIRE(sizeof(u8) == 1);
    REQUIRE(sizeof(u16) == 2);
    REQUIRE(sizeof(u32) == 4);
    REQUIRE(sizeof(u64) == 8);
  }

  SECTION("Floating point types have correct sizes") {
    REQUIRE(sizeof(f32) == 4);
    REQUIRE(sizeof(f64) == 8);
  }
}

TEST_CASE("FixedPoint basic operations", "[fixedpoint]") {
  SECTION("Construction from integer") {
    auto fp = FixedPoint::fromInt(42);
    REQUIRE(fp.toInt() == 42);
  }

  SECTION("Construction from double") {
    auto fp = FixedPoint::fromDouble(3.14);
    REQUIRE(fp.toDouble() >= 3.13);
    REQUIRE(fp.toDouble() <= 3.15);
  }

  SECTION("Addition") {
    auto a = FixedPoint::fromInt(10);
    auto b = FixedPoint::fromInt(20);
    auto c = a + b;
    REQUIRE(c.toInt() == 30);
  }

  SECTION("Subtraction") {
    auto a = FixedPoint::fromInt(50);
    auto b = FixedPoint::fromInt(20);
    auto c = a - b;
    REQUIRE(c.toInt() == 30);
  }

  SECTION("Comparison") {
    auto a = FixedPoint::fromInt(10);
    auto b = FixedPoint::fromInt(20);

    REQUIRE(a < b);
    REQUIRE(b > a);
    REQUIRE(a == FixedPoint::fromInt(10));
    REQUIRE(a != b);
  }
}

TEST_CASE("Arena allocator", "[arena]") {
  SECTION("Arena can be created") {
    Arena arena(1024);
    REQUIRE(arena.bytesCapacity() == 1024);
    REQUIRE(arena.bytesUsed() == 0);
    REQUIRE(arena.bytesRemaining() == 1024);
  }

  SECTION("Arena can allocate memory") {
    Arena arena(1024);
    int* ptr = arena.alloc<int>(10);
    REQUIRE(ptr != nullptr);

    // Memory usage is tracked
    REQUIRE(arena.bytesUsed() > 0);
    REQUIRE(arena.bytesUsed() <= arena.bytesCapacity());
  }

  SECTION("Arena allocations are aligned") {
    Arena arena(1024);
    double* dptr = arena.alloc<double>(1);
    REQUIRE(dptr != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(dptr) % alignof(double) == 0);

    int* iptr = arena.alloc<int>(1);
    REQUIRE(iptr != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(iptr) % Arena::kAlignment == 0);
  }

  SECTION("Arena can be reset") {
    Arena arena(1024);
    arena.alloc<int>(10);
    size_t usedBefore = arena.bytesUsed();
    REQUIRE(usedBefore > 0);

    arena.reset();
    REQUIRE(arena.bytesUsed() == 0);
    REQUIRE(arena.bytesRemaining() == arena.bytesCapacity());
  }

  SECTION("Arena allocations are sequential") {
    Arena arena(1024);
    int* ptr1 = arena.alloc<int>(1);
    int* ptr2 = arena.alloc<int>(1);

    REQUIRE(ptr1 != nullptr);
    REQUIRE(ptr2 != nullptr);
    REQUIRE(ptr1 != ptr2);
  }

  SECTION("ArenaScope sets thread-local arena") {
    Arena arena(1024);
    REQUIRE(threadArena == nullptr);

    {
      ArenaScope scope(arena);
      REQUIRE(threadArena == &arena);
    }

    REQUIRE(threadArena == nullptr);
  }

  SECTION("ArenaScope can be nested") {
    Arena arena1(1024);
    Arena arena2(1024);

    REQUIRE(threadArena == nullptr);

    {
      ArenaScope scope1(arena1);
      REQUIRE(threadArena == &arena1);

      {
        ArenaScope scope2(arena2);
        REQUIRE(threadArena == &arena2);
      }

      REQUIRE(threadArena == &arena1);
    }

    REQUIRE(threadArena == nullptr);
  }
}
