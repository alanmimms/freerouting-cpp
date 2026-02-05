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

TEST_CASE("Arena allocator stub", "[arena]") {
  SECTION("Arena can be created") {
    Arena arena(1024);
    REQUIRE(arena.bytesCapacity() == 1024);
    REQUIRE(arena.bytesUsed() == 0);
  }

  SECTION("Arena can allocate memory (stub)") {
    Arena arena(1024);
    int* ptr = arena.alloc<int>(10);
    REQUIRE(ptr != nullptr);

    // Note: in Phase 1 we'll properly track usage
    // For now, stub uses malloc so usage isn't tracked
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
}
