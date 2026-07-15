#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/renderer/text/TextLayout.hpp"

using engine::Glyph;
using engine::GlyphStore;
using engine::layoutText;

namespace {
GlyphStore makeStore() {
  GlyphStore m;
  // 'A': 6x7, bearing (1,7), advance 8, uv (0,0)-(0.5,0.5)
  m[U'A'] = Glyph{{6, 7}, {1, 7}, 8.0f, {0.0f, 0.0f}, {0.5f, 0.5f}, 0};
  m[U'B'] = Glyph{{6, 7}, {1, 7}, 8.0f, {0.5f, 0.0f}, {1.0f, 0.5f}, 0};
  m[U' '] = Glyph{{0, 0}, {0, 0}, 5.0f, {0.0f, 0.0f}, {0.0f, 0.0f}, 0};
  return m;
}
}  // namespace

TEST_CASE("layoutText places one glyph in local coords by bearing", "[text]") {
  const auto q = layoutText(makeStore(), "A");
  REQUIRE(q.size() == 1);
  REQUIRE(q[0].x0 == 1.0f);   // pen 0 + bearing.x
  REQUIRE(q[0].y0 == -7.0f);  // -bearing.y (above baseline)
  REQUIRE(q[0].x1 == 7.0f);   // x0 + size.x
  REQUIRE(q[0].y1 == 0.0f);   // y0 + size.y
  REQUIRE(q[0].u0 == 0.0f);
  REQUIRE(q[0].u1 == 0.5f);
  REQUIRE(q[0].v1 == 0.5f);
}

TEST_CASE("layoutText advances the pen between glyphs", "[text]") {
  const auto q = layoutText(makeStore(), "AB");
  REQUIRE(q.size() == 2);
  REQUIRE(q[1].x0 == 9.0f);  // advance 8 + bearing.x 1
}

TEST_CASE("layoutText advances on space but emits no quad", "[text]") {
  const auto q = layoutText(makeStore(), "A B");
  REQUIRE(q.size() == 2);     // A and B, not space
  REQUIRE(q[1].x0 == 14.0f);  // 8 (A) + 5 (space) + 1 (B bearing)
}

TEST_CASE("layoutText skips empty and unmapped input", "[text]") {
  REQUIRE(layoutText(makeStore(), "").empty());
  REQUIRE(layoutText(makeStore(), "~").empty());  // '~' absent, no hook
}

TEST_CASE("layoutText uses the missing-glyph hook when provided", "[text]") {
  const GlyphStore store = makeStore();
  const auto q = layoutText(store, "~", [&](char32_t) {
    return &store.at(U'A');  // substitute 'A' for any missing codepoint
  });
  REQUIRE(q.size() == 1);
  REQUIRE(q[0].x1 == 7.0f);
}
