#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <glm/glm.hpp>

#include "engine/renderer/PointLight.hpp"

using engine::pointLightRing;

TEST_CASE("pointLightRing places lights on a circle at a fixed height",
          "[light]") {
  const glm::vec3 p = pointLightRing(0, 8, 0.0f, 6.0f, 1.5f);
  REQUIRE(p.y == Catch::Approx(1.5f));
  REQUIRE(std::sqrt(p.x * p.x + p.z * p.z) == Catch::Approx(6.0f));
}

TEST_CASE("pointLightRing spaces consecutive lights by 2pi/count", "[light]") {
  const glm::vec3 a = pointLightRing(0, 8, 0.0f, 6.0f, 1.5f);
  const glm::vec3 b = pointLightRing(1, 8, 0.0f, 6.0f, 1.5f);
  const float angleA = std::atan2(a.z, a.x);
  const float angleB = std::atan2(b.z, b.x);
  REQUIRE((angleB - angleA) == Catch::Approx(6.2831853f / 8.0f).margin(1e-4));
}

TEST_CASE("pointLightRing rotates the whole ring by time", "[light]") {
  const glm::vec3 a = pointLightRing(0, 8, 0.0f, 6.0f, 1.5f);
  const glm::vec3 t = pointLightRing(0, 8, 0.5f, 6.0f, 1.5f);
  const float angleA = std::atan2(a.z, a.x);
  const float angleT = std::atan2(t.z, t.x);
  REQUIRE((angleT - angleA) == Catch::Approx(0.5f).margin(1e-4));
}
