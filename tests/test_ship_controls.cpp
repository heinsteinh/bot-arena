#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/gameplay/ShipControls.hpp"

using engine::circlesOverlapXZ;
using engine::forwardFromYaw;
using engine::headingToYaw;

TEST_CASE("headingToYaw maps a direction to a yaw", "[ship]") {
  REQUIRE(headingToYaw({0, 0, 1}) == Catch::Approx(0.0f));
  REQUIRE(headingToYaw({1, 0, 0}) == Catch::Approx(1.5707963f));   // +pi/2
  REQUIRE(headingToYaw({0, 0, -1}) == Catch::Approx(3.1415927f));  // pi
}

TEST_CASE("forwardFromYaw is the inverse on XZ", "[ship]") {
  const glm::vec3 f0 = forwardFromYaw(0.0f);
  REQUIRE(f0.x == Catch::Approx(0.0f).margin(1e-6));
  REQUIRE(f0.z == Catch::Approx(1.0f));
  const glm::vec3 back = forwardFromYaw(headingToYaw({3, 0, 4}));
  REQUIRE(back.x == Catch::Approx(0.6f));  // normalize(3,4)
  REQUIRE(back.z == Catch::Approx(0.8f));
}

TEST_CASE("circlesOverlapXZ ignores Y and uses radii sum", "[ship]") {
  REQUIRE(
      circlesOverlapXZ({0, 0, 0}, 0.5f, {0.5f, 9, 0}, 0.5f));  // dist .5 < 1
  REQUIRE_FALSE(circlesOverlapXZ({0, 0, 0}, 0.5f, {2, 0, 0}, 0.5f));
}
