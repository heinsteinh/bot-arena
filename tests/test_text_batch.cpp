#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

#include "engine/renderer/Texture2D.hpp"
#include "engine/renderer/text/TextRenderer.hpp"
#include "engine/renderer/text/TextSpan.hpp"

using namespace engine;

namespace {
class StubTexture : public Texture2D {
 public:
  explicit StubTexture(uint32_t id) : m_id(id) {}
  void setData(const void*, uint32_t) override {}
  void bind(uint32_t) const override {}
  uint32_t rendererID() const override { return m_id; }
  uint32_t width() const override { return 1; }
  uint32_t height() const override { return 1; }

 private:
  uint32_t m_id;
};

FontAsset makeFont(uint32_t atlasId) {
  FontAsset f;
  f.backend = FontBackend::Bitmap;
  f.atlas = CreateRef<GlyphAtlas>(CreateRef<StubTexture>(atlasId), 64, 64);
  // 'A' 10x10, bearing (0,10), advance 12
  f.glyphs[U'A'] = Glyph{{10, 10}, {0, 10}, 12, {0, 0}, {0.5f, 0.5f}, 0};
  f.glyphs[U'B'] = Glyph{{10, 10}, {0, 10}, 12, {0.5f, 0.0f}, {1.0f, 0.5f}, 0};
  f.glyphs[U'V'] = Glyph{{10, 10}, {0, 10}, 12, {0.0f, 0.5f}, {0.5f, 1.0f}, 0};
  f.glyphs[U' '] = Glyph{{0, 0}, {0, 0}, 6, {0, 0}, {0, 0}, 0};  // advance only
  return f;
}

bool samePositions(const std::vector<TextVertex>& a,
                   const std::vector<TextVertex>& b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i].pos != b[i].pos) return false;  // glm::vec3 operator==
  }
  return true;
}
}  // namespace

TEST_CASE("TextRenderer emits 6 verts per visible glyph", "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(3u);
  TextPlacement p;  // ScreenSpace, pos (0,0), scale 1
  tr.submit(f, "AA", p, TextStyle{}, 800, 600);
  const auto& batches = tr.batches();
  REQUIRE(batches.size() == 1);
  REQUIRE(batches[0].atlas == 3u);
  REQUIRE(batches[0].verts.size() == 12);  // 2 glyphs * 6
}

TEST_CASE("TextRenderer merges same-atlas submissions", "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(5u);
  tr.submit(f, "A", TextPlacement{}, TextStyle{}, 800, 600);
  tr.submit(f, "A", TextPlacement{}, TextStyle{}, 800, 600);
  REQUIRE(tr.batches().size() == 1);
  REQUIRE(tr.batches()[0].verts.size() == 12);
}

TEST_CASE("TextRenderer separates different atlases", "[textbatch]") {
  TextRenderer tr;
  tr.submit(makeFont(1u), "A", TextPlacement{}, TextStyle{}, 800, 600);
  tr.submit(makeFont(2u), "A", TextPlacement{}, TextStyle{}, 800, 600);
  REQUIRE(tr.batches().size() == 2);
}

TEST_CASE("TextRenderer projects a top-left glyph near NDC (-1, +1)",
          "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(1u);
  TextPlacement p;
  p.pos = {0.0f, 10.0f};  // baseline at y=10 so glyph top is y=0
  tr.submit(f, "A", p, TextStyle{}, 800, 600);
  const TextVertex& v0 = tr.batches()[0].verts[0];
  REQUIRE(v0.pos.x == -1.0f);  // x=0 -> NDC -1
  REQUIRE(v0.pos.y == 1.0f);   // y=0 -> NDC +1
  REQUIRE(v0.pos.z == 0.0f);
}

TEST_CASE("TextRenderer clear resets batches", "[textbatch]") {
  TextRenderer tr;
  tr.submit(makeFont(1u), "A", TextPlacement{}, TextStyle{}, 800, 600);
  tr.clear();
  REQUIRE(tr.batches().empty());
}

TEST_CASE("TextRenderer records backend and pxRange on the batch",
          "[textbatch]") {
  TextRenderer tr;
  FontAsset f = makeFont(9u);
  f.backend = FontBackend::SDF;
  f.pxRange = 8.0f;
  tr.submit(f, "A", TextPlacement{}, TextStyle{}, 800, 600);
  REQUIRE(tr.batches().size() == 1);
  REQUIRE(tr.batches()[0].backend == FontBackend::SDF);
  REQUIRE(tr.batches()[0].pxRange == 8.0f);
}

TEST_CASE("TextRenderer dedups equal styles into one table entry",
          "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(20u);
  TextStyle s;
  s.fillColor = {1, 0, 0, 1};
  s.outlineWidthPx = 2.0f;
  tr.submit(f, "A", TextPlacement{}, s, 800, 600);
  tr.submit(f, "A", TextPlacement{}, s, 800, 600);  // same style
  REQUIRE(tr.batches().size() == 1);
  REQUIRE(tr.batches()[0].styles.size() == 1);
  for (const auto& v : tr.batches()[0].verts) REQUIRE(v.styleIndex == 0u);
}

TEST_CASE("TextRenderer gives distinct styles distinct indices",
          "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(21u);
  TextStyle a;
  a.fillColor = {1, 0, 0, 1};
  TextStyle b;
  b.fillColor = {0, 1, 0, 1};
  tr.submit(f, "A", TextPlacement{}, a, 800, 600);
  tr.submit(f, "A", TextPlacement{}, b, 800, 600);
  REQUIRE(tr.batches().size() == 1);
  REQUIRE(tr.batches()[0].styles.size() == 2);
  // 6 verts for style a (index 0), then 6 for style b (index 1)
  REQUIRE(tr.batches()[0].verts.front().styleIndex == 0u);
  REQUIRE(tr.batches()[0].verts.back().styleIndex == 1u);
}

TEST_CASE("TextRenderer splits a batch past the style cap", "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(22u);
  for (int i = 0; i < 65; ++i) {
    TextStyle s;
    s.fillColor = {static_cast<float>(i) / 64.0f, 0, 0, 1};  // 65 distinct
    tr.submit(f, "A", TextPlacement{}, s, 800, 600);
  }
  REQUIRE(tr.batches().size() == 2);
  REQUIRE(tr.batches()[0].styles.size() == 64);
  REQUIRE(tr.batches()[1].styles.size() == 1);
}

TEST_CASE("rich: distinct styles get distinct indices at the boundary",
          "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(30u);
  TextStyle s1;
  s1.fillColor = {1, 0, 0, 1};
  TextStyle s2;
  s2.fillColor = {0, 1, 0, 1};
  std::vector<TextSpan> spans{{"A", s1}, {"B", s2}};
  tr.submit(f, spans, TextPlacement{}, 800, 600);
  REQUIRE(tr.batches().size() == 1);
  REQUIRE(tr.batches()[0].styles.size() == 2);
  const auto& v = tr.batches()[0].verts;
  REQUIRE(v.size() == 12);
  REQUIRE(v.front().styleIndex == 0u);  // A
  REQUIRE(v.back().styleIndex == 1u);   // B
}

TEST_CASE("rich: equal styles dedup to one entry", "[textbatch]") {
  TextRenderer tr;
  const FontAsset f = makeFont(31u);
  TextStyle s;
  s.fillColor = {1, 1, 0, 1};
  std::vector<TextSpan> spans{{"A", s}, {"B", s}};
  tr.submit(f, spans, TextPlacement{}, 800, 600);
  REQUIRE(tr.batches()[0].styles.size() == 1);
  REQUIRE(tr.batches()[0].verts.front().styleIndex == 0u);
  REQUIRE(tr.batches()[0].verts.back().styleIndex == 0u);
}

TEST_CASE("rich: trailing space in a span advances the next span",
          "[textbatch]") {
  const FontAsset f = makeFont(32u);
  TextStyle s;
  TextRenderer rich;
  std::vector<TextSpan> spans{{"A ", s}, {"B", s}};
  rich.submit(f, spans, TextPlacement{}, 800, 600);
  TextRenderer single;
  single.submit(f, "A B", TextPlacement{}, s, 800, 600);
  REQUIRE(samePositions(rich.batches()[0].verts, single.batches()[0].verts));
}

TEST_CASE("rich: split equals whole (AV)", "[textbatch]") {
  const FontAsset f = makeFont(33u);
  TextStyle s;
  TextRenderer split;
  std::vector<TextSpan> spans{{"A", s}, {"V", s}};
  split.submit(f, spans, TextPlacement{}, 800, 600);
  TextRenderer whole;
  whole.submit(f, "AV", TextPlacement{}, s, 800, 600);
  REQUIRE(samePositions(split.batches()[0].verts, whole.batches()[0].verts));
}

TEST_CASE("rich: empty spans move nothing and allocate no style",
          "[textbatch]") {
  const FontAsset f = makeFont(34u);
  TextStyle s;
  TextRenderer tr;
  std::vector<TextSpan> spans{{"", s}, {"A", s}};
  tr.submit(f, spans, TextPlacement{}, 800, 600);
  REQUIRE(tr.batches().size() == 1);
  REQUIRE(tr.batches()[0].styles.size() == 1);
  TextRenderer justA;
  justA.submit(f, "A", TextPlacement{}, s, 800, 600);
  REQUIRE(samePositions(tr.batches()[0].verts, justA.batches()[0].verts));

  TextRenderer none;
  std::vector<TextSpan> onlyEmpty{{"", s}};
  none.submit(f, onlyEmpty, TextPlacement{}, 800, 600);
  REQUIRE(none.batches().empty());
}

TEST_CASE("rich: missing glyphs across spans equal the single string",
          "[textbatch]") {
  const FontAsset f = makeFont(35u);  // '~' is absent -> skipped
  TextStyle s;
  TextRenderer rich;
  std::vector<TextSpan> spans{{"A~", s}, {"~B", s}};
  rich.submit(f, spans, TextPlacement{}, 800, 600);
  TextRenderer single;
  single.submit(f, "A~~B", TextPlacement{}, s, 800, 600);
  REQUIRE(samePositions(rich.batches()[0].verts, single.batches()[0].verts));
}

TEST_CASE("rich: single-style equals one-span rich", "[textbatch]") {
  const FontAsset f = makeFont(36u);
  TextStyle s;
  s.fillColor = {0.2f, 0.4f, 0.9f, 1};
  TextRenderer one;
  std::vector<TextSpan> spans{{"AB", s}};
  one.submit(f, spans, TextPlacement{}, 800, 600);
  TextRenderer single;
  single.submit(f, "AB", TextPlacement{}, s, 800, 600);
  REQUIRE(samePositions(one.batches()[0].verts, single.batches()[0].verts));
  REQUIRE(one.batches()[0].styles.size() == single.batches()[0].styles.size());
}

TEST_CASE("rich: over 64 styles flushes into another batch", "[textbatch]") {
  const FontAsset f = makeFont(37u);
  std::vector<TextSpan> spans;
  for (int i = 0; i < 65; ++i) {
    TextStyle s;
    s.fillColor = {static_cast<float>(i) / 64.0f, 0, 0, 1};  // 65 distinct
    spans.push_back({"A", s});
  }
  TextRenderer tr;
  tr.submit(f, spans, TextPlacement{}, 800, 600);
  REQUIRE(tr.batches().size() == 2);
  REQUIRE(tr.batches()[0].styles.size() == 64);
  REQUIRE(tr.batches()[1].styles.size() == 1);
}

TEST_CASE("rich: layout state continues across a boundary with a control char",
          "[textbatch]") {
  const FontAsset f = makeFont(38u);  // '\n' absent -> skipped, pen continues
  TextStyle s;
  TextRenderer rich;
  std::vector<TextSpan> spans{{"A\n", s}, {"B", s}};
  rich.submit(f, spans, TextPlacement{}, 800, 600);
  TextRenderer single;
  single.submit(f, "A\nB", TextPlacement{}, s, 800, 600);
  REQUIRE(samePositions(rich.batches()[0].verts, single.batches()[0].verts));
}
