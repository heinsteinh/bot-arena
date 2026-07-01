#include "game/BotArenaGame.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/Input.hpp"
#include "engine/renderer/DebugRenderer.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace game {

void BotArenaGame::onAttach() {
  spdlog::info("BotArenaGame attached");

  m_flyController.setPose({8.0f, 7.0f, 8.0f}, -135.0f, -30.0f);

  m_orbitController.setTarget({0.0f, 0.5f, 0.0f});
  m_orbitController.setOrbit(45.0f, 30.0f, 14.0f);

  m_topDownCamera.lookAt({0.0f, 15.0f, 0.01f}, {0.0f, 0.0f, 0.0f});

  m_minimapCam.lookAt({0.0f, 15.0f, 0.01f}, {0.0f, 0.0f, 0.0f});
  m_minimapCam.setBounds(-12.0f, 12.0f, -12.0f, 12.0f, -100.0f, 100.0f);
}

void BotArenaGame::onDetach() { spdlog::info("BotArenaGame detached"); }

void BotArenaGame::cycleCameraMode() {
  switch (m_cameraMode) {
    case CameraMode::Fly:
      m_cameraMode = CameraMode::Orbit;
      spdlog::info("Camera mode: Orbit");
      break;
    case CameraMode::Orbit:
      m_cameraMode = CameraMode::TopDown;
      spdlog::info("Camera mode: TopDown");
      break;
    case CameraMode::TopDown:
      m_cameraMode = CameraMode::Fly;
      spdlog::info("Camera mode: Fly");
      break;
  }
}

void BotArenaGame::onUpdate(float dt) {
  m_time += dt;

  if (engine::Input::wasKeyPressed(engine::Key::Space)) {
    cycleCameraMode();
  }

  switch (m_cameraMode) {
    case CameraMode::Fly:
      m_flyController.update(dt);
      break;
    case CameraMode::Orbit:
      m_orbitController.update(dt);
      break;
    case CameraMode::TopDown:
      break;
  }
}

namespace {

glm::mat4 botTransform(std::size_t index, float time) {
  constexpr int kSide = 46;  // ceil(sqrt(2048))
  const float spacing = 14.0f / kSide;
  const int row = static_cast<int>(index) / kSide;
  const int col = static_cast<int>(index) % kSide;
  const float x = (col - kSide / 2) * spacing;
  const float z = (row - kSide / 2) * spacing;
  const float y =
      0.4f + 0.25f * std::sin(time * 2.0f + static_cast<float>(index) * 0.15f);
  glm::mat4 t = glm::translate(glm::mat4(1.0f), {x, y, z});
  return glm::scale(t, glm::vec3(0.15f));  // unit cube [-1,1] -> ~0.3 extent
}

}  // namespace

void BotArenaGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    m_wallMat =
        renderer.registry().registerMaterial({{0.7f, 0.7f, 0.7f, 1.0f}, s});
    m_swarmMats[0] =
        renderer.registry().registerMaterial({{0.9f, 0.3f, 0.2f, 1.0f}, s});
    m_swarmMats[1] =
        renderer.registry().registerMaterial({{0.2f, 0.7f, 0.9f, 1.0f}, s});
    m_swarmMats[2] =
        renderer.registry().registerMaterial({{0.5f, 0.9f, 0.3f, 1.0f}, s});
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_flyController.resize(aspect);
  m_orbitController.resize(aspect);
  m_topDownCamera.setBounds(-10.0f * aspect, 10.0f * aspect, -10.0f, 10.0f,
                            -100.0f, 100.0f);

  const engine::Camera* camera = &m_flyController.camera();
  if (m_cameraMode == CameraMode::Orbit) camera = &m_orbitController.camera();
  if (m_cameraMode == CameraMode::TopDown) camera = &m_topDownCamera;

  renderer.setCamera(*camera);
  renderer.setMinimapCamera(m_minimapCam);

  const engine::MeshHandle cube = renderer.unitCubeMesh();

  // Serial: arena walls (main lane) + grid.
  engine::DebugRenderer debug(renderer.queue());
  engine::MeshRenderer walls(renderer.queue(), renderer.registry(), *camera);
  debug.drawGrid(10.0f, 1.0f, {0.25f, 0.25f, 0.25f, 1.0f});
  auto wall = [&](const glm::vec3& center, const glm::vec3& size) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), center);
    t = glm::scale(t, size * 0.5f);
    walls.submit(cube, m_wallMat, t);
  };
  wall({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f});
  wall({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f});
  wall({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});
  wall({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});

  // Parallel: the bot swarm.
  const float time = m_time;
  renderer.generateMeshes(kBotCount, [this, &renderer, camera, cube, time](
                                         engine::RenderQueue& q,
                                         std::size_t begin, std::size_t end) {
    engine::MeshRenderer mesh(q, renderer.registry(), *camera);
    for (std::size_t i = begin; i < end; ++i) {
      mesh.submit(cube, m_swarmMats[i % 3], botTransform(i, time));
    }
  });
}

}  // namespace game
