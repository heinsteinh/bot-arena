#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/RenderCommand.hpp"

using engine::makeSortKey;
using engine::RenderLayer;

TEST_CASE("makeSortKey packs layer into the high 8 bits", "[sortkey]") {
  REQUIRE((makeSortKey(RenderLayer::Opaque, 5) >> 56) ==
          static_cast<uint64_t>(RenderLayer::Opaque));
  REQUIRE((makeSortKey(RenderLayer::Grid, 0) & 0x00FFFFFFFFFFFFFFull) == 0);
  REQUIRE((makeSortKey(RenderLayer::Debug, 42) & 0x00FFFFFFFFFFFFFFull) == 42);
}

TEST_CASE("makeSortKey orders by layer then sequence", "[sortkey]") {
  REQUIRE(makeSortKey(RenderLayer::Grid, 100) <
          makeSortKey(RenderLayer::Opaque, 0));
  REQUIRE(makeSortKey(RenderLayer::Opaque, 0) <
          makeSortKey(RenderLayer::Debug, 0));
  REQUIRE(makeSortKey(RenderLayer::Debug, 1) <
          makeSortKey(RenderLayer::Debug, 2));
}
