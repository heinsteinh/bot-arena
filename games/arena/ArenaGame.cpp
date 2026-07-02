#include "games/arena/ArenaGame.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace arena {

void ArenaGame::onAttach() {
  m_camera.setTarget({0.0f, 0.5f, 0.0f});
  m_camera.setOrbit(45.0f, 40.0f, 16.0f);
}

void ArenaGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    m_wallMat = renderer.registry().registerMaterial(
        {{0.7f, 0.7f, 0.7f, 1.0f}, 0.0f, 0.5f, s});
    m_groundMat = renderer.registry().registerMaterial(
        {{0.3f, 0.3f, 0.33f, 1.0f}, 0.0f, 0.85f, s});
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_camera.resize(aspect);
  renderer.setCamera(m_camera.camera());

  std::vector<engine::PointLight> lights;
  const glm::vec3 palette[4] = {{1.0f, 0.3f, 0.2f},
                                {0.3f, 0.6f, 1.0f},
                                {0.4f, 1.0f, 0.4f},
                                {1.0f, 0.9f, 0.3f}};
  for (int i = 0; i < 4; ++i) {
    engine::PointLight pl;
    const float sx = (i & 1) ? 4.0f : -4.0f;
    const float sz = (i & 2) ? 4.0f : -4.0f;
    pl.positionRadius = glm::vec4(sx, 2.0f, sz, 8.0f);
    pl.color = glm::vec4(palette[i], 3.0f);
    lights.push_back(pl);
  }
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(),
                              m_camera.camera());
  auto wall = [&](const glm::vec3& center, const glm::vec3& size) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), center);
    t = glm::scale(t, size * 0.5f);
    meshes.submit(cube, m_wallMat, t);
  };
  wall({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f});
  wall({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f});
  wall({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});
  wall({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.05f, 0.0f});
  ground = glm::scale(ground, {20.0f, 0.05f, 20.0f});
  meshes.submit(cube, m_groundMat, ground);
}

}  // namespace arena
