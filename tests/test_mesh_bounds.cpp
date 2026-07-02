#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/assets/MeshBounds.hpp"

using engine::AABB;
using engine::computeBounds;
using engine::fitToUnitTransform;

TEST_CASE("computeBounds finds the min and max corners", "[bounds]") {
  const glm::vec3 pts[] = {{-1, 2, -3}, {4, -5, 6}, {0, 0, 0}};
  const AABB b = computeBounds(pts, 3);
  REQUIRE(b.min == glm::vec3(-1, -5, -3));
  REQUIRE(b.max == glm::vec3(4, 2, 6));
}

TEST_CASE("computeBounds on no points is a zero box", "[bounds]") {
  const AABB b = computeBounds(nullptr, 0);
  REQUIRE(b.min == glm::vec3(0));
  REQUIRE(b.max == glm::vec3(0));
}

TEST_CASE("fitToUnitTransform centers and scales to unit", "[bounds]") {
  AABB b;
  b.min = {0, 0, 0};
  b.max = {2, 0, 0};  // center (1,0,0), largest extent 2 -> scale 0.5
  const glm::mat4 m = fitToUnitTransform(b);
  const glm::vec4 center = m * glm::vec4(1, 0, 0, 1);
  const glm::vec4 corner = m * glm::vec4(2, 0, 0, 1);
  REQUIRE(center.x == Catch::Approx(0.0f));
  REQUIRE(corner.x == Catch::Approx(0.5f));
}
