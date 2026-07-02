#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/physics/Collision.hpp"

using engine::resolveWallBounce;

TEST_CASE("resolveWallBounce leaves interior points untouched", "[physics]") {
  const auto r =
      resolveWallBounce({0, 0, 0}, {1, 0, -1}, {-5, -5, -5}, {5, 5, 5}, 0.5f);
  REQUIRE(r.position == glm::vec3(0, 0, 0));
  REQUIRE(r.velocity == glm::vec3(1, 0, -1));
}

TEST_CASE("resolveWallBounce clamps and reflects past the max wall",
          "[physics]") {
  const auto r =
      resolveWallBounce({6, 0, 0}, {2, 0, 0}, {-5, -5, -5}, {5, 5, 5}, 0.5f);
  REQUIRE(r.position.x == 4.5f);   // clamped to max - radius
  REQUIRE(r.velocity.x == -2.0f);  // reflected inward
}

TEST_CASE("resolveWallBounce clamps and reflects past the min wall",
          "[physics]") {
  const auto r =
      resolveWallBounce({0, 0, -6}, {0, 0, -2}, {-5, -5, -5}, {5, 5, 5}, 0.5f);
  REQUIRE(r.position.z == -4.5f);
  REQUIRE(r.velocity.z == 2.0f);
}
