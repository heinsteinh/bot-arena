#include <catch2/catch_test_macros.hpp>
#include <unordered_map>

#include "engine/renderer/text/FontDesc.hpp"

using engine::FontBackend;
using engine::FontDesc;
using engine::FontDescHash;

namespace {
FontDesc base() {
  FontDesc d;
  d.family = "a.ttf";
  d.pixelSize = 32;
  d.backend = FontBackend::Bitmap;
  return d;
}
}  // namespace

TEST_CASE("FontDesc equal descriptors compare and hash equal", "[fontdesc]") {
  const FontDesc a = base();
  const FontDesc b = base();
  REQUIRE(a == b);
  REQUIRE(FontDescHash{}(a) == FontDescHash{}(b));
}

TEST_CASE("FontDesc differs on size or backend", "[fontdesc]") {
  FontDesc a = base();
  FontDesc big = base();
  big.pixelSize = 48;
  FontDesc msdf = base();
  msdf.backend = FontBackend::MSDF;
  REQUIRE_FALSE(a == big);
  REQUIRE_FALSE(a == msdf);
}

TEST_CASE("FontDesc works as an unordered_map key (dedup)", "[fontdesc]") {
  std::unordered_map<FontDesc, int, FontDescHash> m;
  m[base()] = 1;
  m[base()] += 1;  // same key
  REQUIRE(m.size() == 1);
  REQUIRE(m[base()] == 2);
}
