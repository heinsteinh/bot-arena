#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/renderer/text/TextVertex.hpp"

using engine::packColor;
using engine::TextPlacement;
using engine::TextStyle;
using engine::TextVertex;

TEST_CASE("TextVertex is tightly packed at 32 bytes", "[textstyle]") {
  REQUIRE(sizeof(TextVertex) == 32);
}

TEST_CASE("packColor packs RGBA8 with red in the low byte", "[textstyle]") {
  REQUIRE(packColor({1.0f, 0.0f, 0.0f, 1.0f}) == 0xFF0000FFu);
  REQUIRE(packColor({0.0f, 1.0f, 0.0f, 1.0f}) == 0xFF00FF00u);
  REQUIRE(packColor({0.0f, 0.0f, 0.0f, 0.0f}) == 0x00000000u);
}

TEST_CASE("packColor clamps out-of-range channels", "[textstyle]") {
  REQUIRE(packColor({2.0f, -1.0f, 0.0f, 1.0f}) == 0xFF0000FFu);
}

TEST_CASE("defaults are sensible", "[textstyle]") {
  TextStyle s;
  REQUIRE(s.fillColor == glm::vec4(1.0f));
  REQUIRE(s.styleIndex == 0u);
  TextPlacement p;
  REQUIRE(p.mode == engine::PlacementMode::ScreenSpace);
  REQUIRE(p.scale == 1.0f);
}

TEST_CASE("toGpuStyle packs effect params into std140 vec4s", "[textstyle]") {
  engine::TextStyle s;
  s.fillColor = {1, 1, 1, 1};
  s.outlineColor = {0, 0, 0, 1};
  s.outlineWidthPx = 3.0f;
  s.glowColor = {0, 1, 1, 1};
  s.glowSizePx = 8.0f;
  s.shadowColor = {0, 0, 0, 0.7f};
  s.shadowOffsetPx = {3.0f, 4.0f};
  s.shadowSoftnessPx = 2.0f;
  const engine::GpuStyle g = engine::toGpuStyle(s);
  REQUIRE(g.fillColor == glm::vec4(1, 1, 1, 1));
  REQUIRE(g.outlineColor == glm::vec4(0, 0, 0, 1));
  REQUIRE(g.glowColor == glm::vec4(0, 1, 1, 1));
  REQUIRE(g.shadowColor == glm::vec4(0, 0, 0, 0.7f));
  REQUIRE(g.params0 ==
          glm::vec4(3.0f, 8.0f, 3.0f, 4.0f));  // outlineW, glowSz, shadowOff.xy
  REQUIRE(g.params1.x == 2.0f);                // shadowSoftness
}

TEST_CASE("GpuStyle is 96 bytes and equality-comparable", "[textstyle]") {
  REQUIRE(sizeof(engine::GpuStyle) == 96);
  engine::GpuStyle a = engine::toGpuStyle(engine::TextStyle{});
  engine::GpuStyle b = engine::toGpuStyle(engine::TextStyle{});
  REQUIRE(a == b);
  engine::TextStyle s2;
  s2.outlineWidthPx = 1.0f;
  REQUIRE_FALSE(a == engine::toGpuStyle(s2));
}

TEST_CASE("default TextStyle has effects off", "[textstyle]") {
  engine::TextStyle s;
  REQUIRE(s.outlineWidthPx == 0.0f);
  REQUIRE(s.glowSizePx == 0.0f);
  REQUIRE(s.shadowOffsetPx == glm::vec2(0.0f));
}
