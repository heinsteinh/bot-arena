#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/renderer/LightUniforms.hpp"

using engine::makeLightViewProj;

TEST_CASE("makeLightViewProj centers the scene in light clip space",
          "[light]") {
  const glm::vec3 center{0.0f, 0.5f, 0.0f};
  const glm::mat4 vp =
      makeLightViewProj({0.4f, 0.8f, 0.3f}, center, 16.0f, 0.1f, 64.0f);
  const glm::vec4 c = vp * glm::vec4(center, 1.0f);
  const glm::vec3 ndc = glm::vec3(c) / c.w;
  REQUIRE(ndc.x == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(ndc.y == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(ndc.z > -1.0f);
  REQUIRE(ndc.z < 1.0f);
}

TEST_CASE("makeLightViewProj: nearer to the light means smaller depth",
          "[light]") {
  const glm::vec3 center{0.0f, 0.5f, 0.0f};
  const glm::vec3 dir = glm::normalize(glm::vec3(0.4f, 0.8f, 0.3f));
  const glm::mat4 vp = makeLightViewProj(dir, center, 16.0f, 0.1f, 64.0f);
  const glm::vec4 near = vp * glm::vec4(center - dir * 5.0f, 1.0f);
  const glm::vec4 far = vp * glm::vec4(center + dir * 5.0f, 1.0f);
  REQUIRE(near.z / near.w < far.z / far.w);
}
