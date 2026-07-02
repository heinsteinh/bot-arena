#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/ai/Steering.hpp"

using engine::flee;
using engine::seek;
using engine::truncate;

TEST_CASE("truncate leaves short vectors and scales long ones", "[steering]") {
  REQUIRE(truncate({1, 0, 0}, 5.0f) == glm::vec3(1, 0, 0));
  const glm::vec3 t = truncate({3, 0, 4}, 2.5f);  // length 5 -> 2.5
  REQUIRE(glm::length(t) == Catch::Approx(2.5f).margin(1e-4));
}

TEST_CASE("seek from rest steers toward the target", "[steering]") {
  const glm::vec3 f = seek({0, 0, 0}, {0, 0, 0}, {10, 0, 0}, 2.0f, 8.0f);
  REQUIRE(f.x == Catch::Approx(2.0f));  // desired 2 along +X, minus zero vel
  REQUIRE(f.y == 0.0f);
  REQUIRE(f.z == 0.0f);
}

TEST_CASE("seek caps the force at maxForce", "[steering]") {
  const glm::vec3 f = seek({0, 0, 0}, {0, 0, -10}, {10, 0, 0}, 2.0f, 8.0f);
  REQUIRE(glm::length(f) == Catch::Approx(8.0f).margin(1e-4));
}

TEST_CASE("seek at the target is zero", "[steering]") {
  const glm::vec3 f = seek({1, 0, 1}, {0, 0, 0}, {1, 0, 1}, 2.0f, 8.0f);
  REQUIRE(f == glm::vec3(0, 0, 0));
}

TEST_CASE("flee steers away from the target", "[steering]") {
  const glm::vec3 f = flee({0, 0, 0}, {0, 0, 0}, {10, 0, 0}, 2.0f, 8.0f);
  REQUIRE(f.x == Catch::Approx(-2.0f));  // away from +X target
  REQUIRE(f.z == 0.0f);
}
