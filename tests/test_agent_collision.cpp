#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <glm/glm.hpp>

#include "engine/physics/Collision.hpp"

using engine::resolveAgentPair;

TEST_CASE("resolveAgentPair leaves a separated pair untouched", "[physics]") {
  const auto r =
      resolveAgentPair({0, 0, 0}, {1, 0, 0}, 0.5f, {3, 0, 0}, {0, 0, 0}, 0.5f);
  REQUIRE(r.posA == glm::vec3(0, 0, 0));
  REQUIRE(r.posB == glm::vec3(3, 0, 0));
  REQUIRE(r.velA == glm::vec3(1, 0, 0));
  REQUIRE(r.velB == glm::vec3(0, 0, 0));
}

TEST_CASE("resolveAgentPair separates and bounces an approaching pair",
          "[physics]") {
  const auto r = resolveAgentPair({0, 0, 0}, {1, 0, 0}, 0.5f, {0.5f, 0, 0},
                                  {-1, 0, 0}, 0.5f);
  // overlap 0.5 -> each pushed 0.25 apart along +X
  REQUIRE(r.posA.x == -0.25f);
  REQUIRE(r.posB.x == 0.75f);
  // approaching -> normal velocities swapped (now moving apart)
  REQUIRE(r.velA.x == -1.0f);
  REQUIRE(r.velB.x == 1.0f);
}

TEST_CASE("resolveAgentPair separates but keeps a receding pair's velocity",
          "[physics]") {
  const auto r = resolveAgentPair({0, 0, 0}, {-1, 0, 0}, 0.5f, {0.5f, 0, 0},
                                  {1, 0, 0}, 0.5f);
  REQUIRE(r.posA.x == -0.25f);
  REQUIRE(r.posB.x == 0.75f);
  REQUIRE(r.velA.x == -1.0f);  // unchanged (already separating)
  REQUIRE(r.velB.x == 1.0f);
}

TEST_CASE("resolveAgentPair pushes coincident centers apart without NaN",
          "[physics]") {
  const auto r =
      resolveAgentPair({0, 0, 0}, {0, 0, 0}, 0.5f, {0, 0, 0}, {0, 0, 0}, 0.5f);
  REQUIRE(r.posA.x == -0.5f);
  REQUIRE(r.posB.x == 0.5f);
  REQUIRE(std::isfinite(r.posA.x));
  REQUIRE(std::isfinite(r.posB.x));
}
