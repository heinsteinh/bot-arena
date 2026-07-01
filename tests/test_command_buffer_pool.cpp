#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/renderer/CommandBufferPool.hpp"

using engine::CreateScope;
using engine::makeSortKey;
using engine::mergeLaneEntries;
using engine::RenderCommand;
using engine::RenderEntry;
using engine::RenderLayer;
using engine::Scope;
using engine::WorkerBuffer;

static RenderCommand cmd(float x) {
  RenderCommand c;
  c.layer = RenderLayer::Opaque;
  c.position = {x, 0.0f, 0.0f};
  c.sortKey = makeSortKey(RenderLayer::Opaque, 0, 0, 0);
  return c;
}

TEST_CASE("mergeLaneEntries concatenates lanes in order", "[pool]") {
  std::vector<Scope<WorkerBuffer>> lanes;
  lanes.push_back(CreateScope<WorkerBuffer>(4096));
  lanes.push_back(CreateScope<WorkerBuffer>(4096));

  lanes[0]->queue.submit(cmd(1.0f));
  lanes[0]->queue.submit(cmd(2.0f));
  lanes[1]->queue.submit(cmd(3.0f));

  std::vector<RenderEntry> out;
  mergeLaneEntries(lanes, out);

  REQUIRE(out.size() == 3);
  REQUIRE(out[0].command->position.x == 1.0f);
  REQUIRE(out[1].command->position.x == 2.0f);
  REQUIRE(out[2].command->position.x == 3.0f);
}
