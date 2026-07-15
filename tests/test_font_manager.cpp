#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/Texture2D.hpp"
#include "engine/renderer/text/FontManager.hpp"

using namespace engine;

namespace {
// Headless Texture2D stub: no GL, just a fixed id.
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

// Counts bake calls so we can assert cache hits.
class CountingSource : public FontSource {
 public:
  int bakes = 0;
  FontBackend backend() const override { return FontBackend::Bitmap; }
  bool bake(const FontDesc&, BakedFont& out) override {
    ++bakes;
    out.atlasWidth = 4;
    out.atlasHeight = 4;
    out.glyphs[U'A'] = Glyph{{2, 2}, {0, 2}, 3, {0, 0}, {0.5f, 0.5f}, 0};
    return true;
  }
};

AtlasFactory stubFactory() {
  return [](const BakedFont& b) {
    return CreateRef<GlyphAtlas>(CreateRef<StubTexture>(7u), b.atlasWidth,
                                 b.atlasHeight);
  };
}

FontDesc desc(uint32_t size = 32) {
  FontDesc d;
  d.family = "x.ttf";
  d.pixelSize = size;
  d.backend = FontBackend::Bitmap;
  return d;
}
}  // namespace

TEST_CASE("FontManager caches by descriptor (bakes once)", "[fontmgr]") {
  FontManager mgr(stubFactory());
  auto src = CreateScope<CountingSource>();
  CountingSource* raw = src.get();
  mgr.registerSource(std::move(src));

  FontHandle a = mgr.load(desc());
  FontHandle b = mgr.load(desc());  // same descriptor
  REQUIRE(a != nullptr);
  REQUIRE(a == b);           // same shared asset
  REQUIRE(raw->bakes == 1);  // baked once
  REQUIRE(a->glyphs.count(U'A') == 1);
  REQUIRE(a->atlasRendererID() == 7u);
}

TEST_CASE("FontManager bakes again for a different descriptor", "[fontmgr]") {
  FontManager mgr(stubFactory());
  auto src = CreateScope<CountingSource>();
  CountingSource* raw = src.get();
  mgr.registerSource(std::move(src));

  mgr.load(desc(32));
  mgr.load(desc(48));  // different size
  REQUIRE(raw->bakes == 2);
}

TEST_CASE("FontManager returns null with no source for backend", "[fontmgr]") {
  FontManager mgr(stubFactory());
  REQUIRE(mgr.load(desc()) == nullptr);
}
