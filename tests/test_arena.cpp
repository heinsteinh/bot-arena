#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "engine/core/Arena.hpp"

using engine::Arena;

TEST_CASE("Arena bumps the offset by allocation size", "[arena]") {
  Arena arena(1024);
  REQUIRE(arena.used() == 0);
  REQUIRE(arena.capacity() == 1024);

  arena.allocate(1, 1);
  REQUIRE(arena.used() == 1);

  arena.allocate(3, 1);
  REQUIRE(arena.used() == 4);
}

TEST_CASE("Arena aligns allocations up", "[arena]") {
  Arena arena(1024);
  arena.allocate(4, 1);                // used == 4
  void* p = arena.alloc<uint64_t>(1);  // align 8: 4 -> 8, then +8
  REQUIRE(reinterpret_cast<std::uintptr_t>(p) % alignof(uint64_t) == 0);
  REQUIRE(arena.used() == 16);
}

TEST_CASE("Arena reset rewinds and reuses", "[arena]") {
  Arena arena(1024);
  void* first = arena.allocate(64, 16);
  arena.reset();
  REQUIRE(arena.used() == 0);
  void* again = arena.allocate(64, 16);
  REQUIRE(again == first);
}

TEST_CASE("Arena throws when out of memory", "[arena]") {
  Arena arena(16);
  REQUIRE_THROWS_AS(arena.allocate(20, 1), std::runtime_error);
}
