#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/text/ShelfPacker.hpp"

using engine::ShelfPacker;

TEST_CASE("ShelfPacker places rects left-to-right on a shelf", "[packer]") {
  ShelfPacker p(64, /*pad=*/1);
  const auto a = p.place(10, 8);
  const auto b = p.place(12, 6);
  REQUIRE(a.x == 1);  // starts after left padding
  REQUIRE(a.y == 1);
  REQUIRE(b.x == 1 + 10 + 1);  // after A + padding
  REQUIRE(b.y == 1);
}

TEST_CASE("ShelfPacker wraps to a new shelf when the row is full", "[packer]") {
  ShelfPacker p(20, 1);
  const auto a = p.place(10, 8);  // row 0
  const auto b = p.place(12, 5);  // doesn't fit (1+10+1+12+1 > 20) -> wrap
  REQUIRE(a.y == 1);
  REQUIRE(b.x == 1);          // back to left
  REQUIRE(b.y == 1 + 8 + 1);  // below tallest of previous row + pad
}

TEST_CASE("ShelfPacker height covers all rows plus padding", "[packer]") {
  ShelfPacker p(20, 1);
  p.place(10, 8);
  p.place(12, 5);  // wraps to row 1 (height 5)
  REQUIRE(p.height() == 1 + 8 + 1 + 5 + 1);
}
