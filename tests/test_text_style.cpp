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
