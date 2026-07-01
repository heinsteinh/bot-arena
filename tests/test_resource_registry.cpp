#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/ResourceRegistry.hpp"

using engine::Material;
using engine::ResourceRegistry;

TEST_CASE("registerMaterial returns increasing handles and stores value",
          "[registry]") {
  ResourceRegistry reg;
  Material red;
  red.baseColor = {1.0f, 0.0f, 0.0f, 1.0f};
  red.shader = 7;
  Material blue;
  blue.baseColor = {0.0f, 0.0f, 1.0f, 1.0f};

  const engine::MaterialHandle h0 = reg.registerMaterial(red);
  const engine::MaterialHandle h1 = reg.registerMaterial(blue);

  REQUIRE(h0 == 0);
  REQUIRE(h1 == 1);
  REQUIRE(reg.material(h0).baseColor.r == 1.0f);
  REQUIRE(reg.material(h0).shader == 7);
  REQUIRE(reg.material(h1).baseColor.b == 1.0f);
}

TEST_CASE("registerMaterial stores metallic and roughness", "[registry]") {
  ResourceRegistry reg;
  Material def;
  REQUIRE(def.metallic == 0.0f);
  REQUIRE(def.roughness == 0.5f);

  Material metal;
  metal.baseColor = {1.0f, 0.8f, 0.3f, 1.0f};
  metal.metallic = 1.0f;
  metal.roughness = 0.2f;
  const engine::MaterialHandle h = reg.registerMaterial(metal);
  REQUIRE(reg.material(h).metallic == 1.0f);
  REQUIRE(reg.material(h).roughness == 0.2f);
}

TEST_CASE("registerMesh/Shader hand out stable increasing handles",
          "[registry]") {
  ResourceRegistry reg;
  // Null Refs are fine for exercising handle bookkeeping (no deref).
  REQUIRE(reg.registerShader(nullptr) == 0);
  REQUIRE(reg.registerShader(nullptr) == 1);
  REQUIRE(reg.registerMesh(nullptr) == 0);
  REQUIRE(reg.registerMesh(nullptr) == 1);
  REQUIRE(reg.shader(0) == nullptr);
  REQUIRE(reg.mesh(1) == nullptr);
}
