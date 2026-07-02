#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "engine/core/FixedTimestep.hpp"

using engine::fixedTimestep;

TEST_CASE("fixedTimestep runs no step below one step", "[timestep]") {
  const auto r = fixedTimestep(0.008f, 1.0f / 60.0f, 5);
  REQUIRE(r.steps == 0);
  REQUIRE(r.remainder == Catch::Approx(0.008f));
}

TEST_CASE("fixedTimestep runs whole steps and keeps the remainder",
          "[timestep]") {
  const auto r = fixedTimestep(0.05f, 1.0f / 60.0f, 5);
  REQUIRE(r.steps == 3);  // 0.05 / 0.01667 = 3
  REQUIRE(r.remainder == Catch::Approx(0.05f - 3.0f / 60.0f).margin(1e-5));
}

TEST_CASE("fixedTimestep caps runaway backlog", "[timestep]") {
  const auto r = fixedTimestep(10.0f, 1.0f / 60.0f, 5);
  REQUIRE(r.steps == 5);
  REQUIRE(r.remainder == 0.0f);
}
