#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/Texture2D.hpp"
#include "engine/renderer/text/TextRenderer.hpp"

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
  return f;
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
