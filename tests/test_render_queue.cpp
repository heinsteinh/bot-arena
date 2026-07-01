#include <catch2/catch_test_macros.hpp>

#include "engine/core/Arena.hpp"
#include "engine/renderer/RenderQueue.hpp"

using engine::Arena;
using engine::RenderCommand;
using engine::RenderLayer;
using engine::RenderQueue;

static RenderCommand cmd(RenderLayer layer) {
  RenderCommand c;
  c.layer = layer;
  return c;
}

TEST_CASE("RenderQueue sorts entries by layer", "[queue]") {
  Arena arena(4096);
  RenderQueue queue(arena);

  queue.submit(cmd(RenderLayer::Debug));
  queue.submit(cmd(RenderLayer::Grid));
  queue.submit(cmd(RenderLayer::Opaque));
  queue.sort();

  const auto& e = queue.entries();
  REQUIRE(e.size() == 3);
  REQUIRE(e[0].command->layer == RenderLayer::Grid);
  REQUIRE(e[1].command->layer == RenderLayer::Opaque);
  REQUIRE(e[2].command->layer == RenderLayer::Debug);
}

TEST_CASE("RenderQueue preserves submission order within a layer", "[queue]") {
  Arena arena(4096);
  RenderQueue queue(arena);

  RenderCommand a = cmd(RenderLayer::Opaque);
  a.position = {1.0f, 0.0f, 0.0f};
  RenderCommand b = cmd(RenderLayer::Opaque);
  b.position = {2.0f, 0.0f, 0.0f};

  queue.submit(a);
  queue.submit(b);
  queue.sort();

  const auto& e = queue.entries();
  REQUIRE(e[0].command->position.x == 1.0f);
  REQUIRE(e[1].command->position.x == 2.0f);
}

TEST_CASE("RenderQueue clear empties entries", "[queue]") {
  Arena arena(4096);
  RenderQueue queue(arena);
  queue.submit(cmd(RenderLayer::Grid));
  queue.clear();
  REQUIRE(queue.entries().empty());
}
