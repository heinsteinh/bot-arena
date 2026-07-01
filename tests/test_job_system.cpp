#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "engine/core/JobSystem.hpp"

using engine::JobSystem;

static void visitAll(JobSystem& js, size_t count) {
  std::atomic<size_t> visits{0};
  std::atomic<uint64_t> sum{0};
  std::atomic<size_t> maxLane{0};
  js.parallelFor(count, 64, [&](size_t begin, size_t end, size_t lane) {
    size_t prev = maxLane.load();
    while (lane > prev && !maxLane.compare_exchange_weak(prev, lane)) {
    }
    for (size_t i = begin; i < end; ++i) {
      visits.fetch_add(1);
      sum.fetch_add(i);
    }
  });
  REQUIRE(visits.load() == count);
  REQUIRE(sum.load() == static_cast<uint64_t>(count) * (count - 1) / 2);
  REQUIRE(maxLane.load() < js.workerCount());
}

TEST_CASE("parallelFor visits every index once (multi-threaded)", "[jobs]") {
  JobSystem js(4);
  REQUIRE(js.workerCount() == 5);
  visitAll(js, 10000);
}

TEST_CASE("parallelFor works serially (zero workers)", "[jobs]") {
  JobSystem js(0);
  REQUIRE(js.workerCount() == 1);
  visitAll(js, 10000);
}

TEST_CASE("parallelFor with zero count is a no-op", "[jobs]") {
  JobSystem js(4);
  bool called = false;
  js.parallelFor(0, 64, [&](size_t, size_t, size_t) { called = true; });
  REQUIRE_FALSE(called);
}

TEST_CASE("parallelFor writes each slot exactly once (no races)", "[jobs]") {
  JobSystem js(4);
  const size_t n = 50000;
  std::vector<size_t> out(n, 0);
  js.parallelFor(n, 128, [&](size_t begin, size_t end, size_t) {
    for (size_t i = begin; i < end; ++i) out[i] = i + 1;
  });
  for (size_t i = 0; i < n; ++i) REQUIRE(out[i] == i + 1);
}
