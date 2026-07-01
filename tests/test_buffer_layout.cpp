#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/Buffer.hpp"

using engine::BufferLayout;
using engine::ShaderDataType;
using engine::ShaderDataTypeSize;

TEST_CASE("ShaderDataTypeSize returns byte sizes", "[buffer]") {
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Float3) == 12);
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Float2) == 8);
}

TEST_CASE("BufferLayout computes offsets and stride", "[buffer]") {
  BufferLayout layout = {
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float3, "a_normal"},
  };
  const auto& e = layout.elements();
  REQUIRE(e.size() == 2);
  REQUIRE(e[0].offset == 0);
  REQUIRE(e[1].offset == 12);
  REQUIRE(e[0].componentCount() == 3);
  REQUIRE(layout.stride() == 24);
}
