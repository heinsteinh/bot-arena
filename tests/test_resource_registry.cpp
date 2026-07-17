#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

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

TEST_CASE("registerModel hands out increasing handles and stores the model",
          "[registry]") {
  ResourceRegistry reg;
  engine::Model a;
  a.submeshes = {{3, 4}, {5, 6}};  // {mesh, material} pairs
  a.bounds.min = {-1, -2, -3};
  a.bounds.max = {1, 2, 3};
  a.valid = true;
  engine::Model b;
  b.submeshes = {{7, 8}};

  const engine::ModelHandle h0 = reg.registerModel(a);
  const engine::ModelHandle h1 = reg.registerModel(b);

  REQUIRE(h0 == 0);
  REQUIRE(h1 == 1);
  REQUIRE(reg.model(h0).submeshes.size() == 2);
  REQUIRE(reg.model(h0).submeshes[1].mesh == 5);
  REQUIRE(reg.model(h0).submeshes[1].material == 6);
  REQUIRE(reg.model(h0).bounds.max == glm::vec3(1, 2, 3));
  REQUIRE(reg.model(h0).valid == true);
  REQUIRE(reg.model(h1).submeshes.size() == 1);
}
