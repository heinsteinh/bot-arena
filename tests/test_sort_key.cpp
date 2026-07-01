#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/RenderCommand.hpp"

using engine::makeSortKey;
using engine::RenderLayer;

TEST_CASE("makeSortKey packs layer, shader, material, depth", "[sortkey]") {
  const uint64_t key =
      makeSortKey(RenderLayer::Opaque, 0x1234, 0xABCD, 0x00FF00);
  REQUIRE((key >> 56) == static_cast<uint64_t>(RenderLayer::Opaque));
  REQUIRE(((key >> 40) & 0xFFFFull) == 0x1234ull);
  REQUIRE(((key >> 24) & 0xFFFFull) == 0xABCDull);
  REQUIRE((key & 0x00FFFFFFull) == 0x00FF00ull);
}

TEST_CASE("makeSortKey masks depth to 24 bits", "[sortkey]") {
  const uint64_t key = makeSortKey(RenderLayer::Opaque, 0, 0, 0xFFFFFFFF);
  REQUIRE((key & 0x00FFFFFFull) == 0x00FFFFFFull);
  REQUIRE(((key >> 24) & 0xFFFFull) == 0ull);  // no bleed into material
}

TEST_CASE("makeSortKey orders layer > shader > material > depth", "[sortkey]") {
  REQUIRE(makeSortKey(RenderLayer::Grid, 0xFFFF, 0xFFFF, 0xFFFFFF) <
          makeSortKey(RenderLayer::Opaque, 0, 0, 0));
  REQUIRE(makeSortKey(RenderLayer::Opaque, 1, 0, 0) <
          makeSortKey(RenderLayer::Opaque, 2, 0, 0));
  REQUIRE(makeSortKey(RenderLayer::Opaque, 1, 1, 0) <
          makeSortKey(RenderLayer::Opaque, 1, 2, 0));
  REQUIRE(makeSortKey(RenderLayer::Opaque, 1, 1, 10) <
          makeSortKey(RenderLayer::Opaque, 1, 1, 20));
}
