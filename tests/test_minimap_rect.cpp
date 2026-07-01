#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/MinimapRect.hpp"

using engine::minimapRect;

TEST_CASE("minimapRect sits in the top-right corner", "[minimap]") {
  const glm::vec4 r = minimapRect(1280, 720, 0.32f, 0.02f);
  REQUIRE(r.z == Catch::Approx(0.98f));  // x1
  REQUIRE(r.w == Catch::Approx(0.98f));  // y1
  REQUIRE(r.x > -1.0f);
  REQUIRE(r.y > -1.0f);
  REQUIRE(r.x < r.z);
  REQUIRE(r.y < r.w);
}

TEST_CASE("minimapRect stays square on screen", "[minimap]") {
  const int w = 1280, h = 720;
  const glm::vec4 r = minimapRect(w, h, 0.32f, 0.02f);
  const float widthPx = (r.z - r.x) * w;
  const float heightPx = (r.w - r.y) * h;
  REQUIRE(widthPx == Catch::Approx(heightPx));
}
