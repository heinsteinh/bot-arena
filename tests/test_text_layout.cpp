#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/renderer/text/TextLayout.hpp"

using engine::Glyph;
using engine::GlyphMap;
using engine::layoutText;

namespace {
GlyphMap makeMap() {
  GlyphMap m{};
  // 'A': 6x7, bearing (1,7), advance 8, uv (0,0)-(0.5,0.5)
  m['A'] = Glyph{{6, 7}, {1, 7}, 8.0f, {0.0f, 0.0f}, {0.5f, 0.5f}};
  // 'B': 6x7, bearing (1,7), advance 8, uv (0.5,0)-(1,0.5)
  m['B'] = Glyph{{6, 7}, {1, 7}, 8.0f, {0.5f, 0.0f}, {1.0f, 0.5f}};
  // space: no bitmap, advance 5
  m[' '] = Glyph{{0, 0}, {0, 0}, 5.0f, {0.0f, 0.0f}, {0.0f, 0.0f}};
  return m;
}
}  // namespace

TEST_CASE("layoutText places one glyph by bearing and copies UVs", "[text]") {
  const auto q = layoutText(makeMap(), "A", 100.0f, 50.0f, 1.0f);
  REQUIRE(q.size() == 1);
  REQUIRE(q[0].x0 == 101.0f);  // 100 + bearing.x
  REQUIRE(q[0].y0 == 43.0f);   // 50 - bearing.y
  REQUIRE(q[0].x1 == 107.0f);  // x0 + size.x
  REQUIRE(q[0].y1 == 50.0f);   // y0 + size.y
  REQUIRE(q[0].u0 == 0.0f);
  REQUIRE(q[0].u1 == 0.5f);
  REQUIRE(q[0].v1 == 0.5f);
}

TEST_CASE("layoutText advances the pen between glyphs", "[text]") {
  const auto q = layoutText(makeMap(), "AB", 0.0f, 0.0f, 1.0f);
  REQUIRE(q.size() == 2);
  REQUIRE(q[1].x0 == 9.0f);  // advance 8 + bearing.x 1
}

TEST_CASE("layoutText scales positions and sizes", "[text]") {
  const auto q = layoutText(makeMap(), "A", 0.0f, 0.0f, 2.0f);
  REQUIRE(q[0].x0 == 2.0f);    // bearing.x*2
  REQUIRE(q[0].y0 == -14.0f);  // -bearing.y*2
  REQUIRE(q[0].x1 == 14.0f);   // 2 + size.x*2
}

TEST_CASE("layoutText advances on space but emits no quad", "[text]") {
  const auto q = layoutText(makeMap(), "A B", 0.0f, 0.0f, 1.0f);
  REQUIRE(q.size() == 2);     // A and B, not space
  REQUIRE(q[1].x0 == 14.0f);  // 8 (A) + 5 (space) + 1 (B bearing)
}

TEST_CASE("layoutText skips empty and unmapped input", "[text]") {
  REQUIRE(layoutText(makeMap(), "", 0.0f, 0.0f, 1.0f).empty());
  REQUIRE(
      layoutText(makeMap(), "~", 0.0f, 0.0f, 1.0f).empty());  // '~' unmapped
}
