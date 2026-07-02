#include <catch2/catch_test_macros.hpp>

#include "engine/gameplay/Combat.hpp"

using engine::adjustHealth;
using engine::shouldFlee;

TEST_CASE("adjustHealth damages, heals, and clamps", "[combat]") {
  REQUIRE(adjustHealth(100.0f, -10.0f, 100.0f) == 90.0f);
  REQUIRE(adjustHealth(5.0f, -10.0f, 100.0f) == 0.0f);    // clamp low
  REQUIRE(adjustHealth(95.0f, 10.0f, 100.0f) == 100.0f);  // clamp high
  REQUIRE(adjustHealth(50.0f, 10.0f, 100.0f) == 60.0f);
}

TEST_CASE("shouldFlee triggers only when alive and low", "[combat]") {
  REQUIRE_FALSE(shouldFlee(30.0f, 30.0f, 0.35f));  // full
  REQUIRE(shouldFlee(10.0f, 30.0f, 0.35f));        // 10 <= 10.5
  REQUIRE_FALSE(shouldFlee(11.0f, 30.0f, 0.35f));  // 11 > 10.5
  REQUIRE_FALSE(shouldFlee(0.0f, 30.0f, 0.35f));   // dead
}
