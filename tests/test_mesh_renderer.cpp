#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/Arena.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"
#include "engine/renderer/RenderQueue.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

using engine::Arena;
using engine::Material;
using engine::MeshRenderer;
using engine::PerspectiveCamera;
using engine::RenderCommandType;
using engine::RenderLayer;
using engine::RenderQueue;
using engine::ResourceRegistry;

TEST_CASE("MeshRenderer submits an Opaque Mesh command with a packed key",
          "[mesh]") {
  Arena arena(4096);
  RenderQueue queue(arena);
  ResourceRegistry reg;
  const engine::MeshHandle mesh = reg.registerMesh(nullptr);
  Material mat;
  mat.shader = 3;
  const engine::MaterialHandle material = reg.registerMaterial(mat);

  PerspectiveCamera cam;
  cam.setPosition({0.0f, 0.0f, 10.0f});
  cam.setRotation(-90.0f, 0.0f);  // look toward -Z

  MeshRenderer renderer(queue, reg, cam);
  renderer.submit(mesh, material, glm::mat4(1.0f));
  queue.sort();

  const auto& e = queue.entries();
  REQUIRE(e.size() == 1);
  REQUIRE(e[0].command->type == RenderCommandType::Mesh);
  REQUIRE(e[0].command->layer == RenderLayer::Opaque);
  REQUIRE((e[0].command->sortKey >> 40 & 0xFFFF) == 3);         // shader handle
  REQUIRE((e[0].command->sortKey >> 24 & 0xFFFF) == material);  // material
}

TEST_CASE("MeshRenderer orders nearer meshes before farther", "[mesh]") {
  Arena arena(4096);
  RenderQueue queue(arena);
  ResourceRegistry reg;
  const engine::MeshHandle mesh = reg.registerMesh(nullptr);
  Material mat;  // shader 0
  const engine::MaterialHandle material = reg.registerMaterial(mat);

  PerspectiveCamera cam;
  cam.setPosition({0.0f, 0.0f, 10.0f});
  cam.setRotation(-90.0f, 0.0f);

  MeshRenderer renderer(queue, reg, cam);
  const glm::mat4 near = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 5.0f});
  const glm::mat4 far = glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -5.0f});
  renderer.submit(mesh, material, far);
  renderer.submit(mesh, material, near);
  queue.sort();

  const auto& e = queue.entries();
  // nearer (smaller depth) sorts first
  REQUIRE((e[0].command->sortKey & 0x00FFFFFF) <
          (e[1].command->sortKey & 0x00FFFFFF));
}
