#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/renderer/SSAOKernel.hpp"

using engine::generateSSAOKernel;

TEST_CASE("generateSSAOKernel returns hemisphere samples", "[ssao]") {
  const auto k = generateSSAOKernel(32);
  REQUIRE(k.size() == 32u);
  for (const glm::vec3& s : k) {
    REQUIRE(s.z >= 0.0f);                // +Z hemisphere
    REQUIRE(glm::length(s) <= 1.0001f);  // within unit sphere
  }
}

TEST_CASE("generateSSAOKernel is deterministic and non-degenerate", "[ssao]") {
  const auto a = generateSSAOKernel(16);
  const auto b = generateSSAOKernel(16);
  REQUIRE(a.size() == b.size());
  bool allSame = true;
  for (size_t i = 0; i < a.size(); ++i) {
    REQUIRE(a[i].x == b[i].x);  // deterministic
    REQUIRE(a[i].y == b[i].y);
    REQUIRE(a[i].z == b[i].z);
    if (a[i] != a[0]) allSame = false;
  }
  REQUIRE_FALSE(allSame);  // not all identical
}
