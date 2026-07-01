#include "game/BotArenaGame.hpp"

#include <spdlog/spdlog.h>

#include <cmath>

#include "engine/core/Input.hpp"
#include "engine/renderer/DebugRenderer.hpp"

namespace game {

void BotArenaGame::onAttach() {
  spdlog::info("BotArenaGame attached");

  m_flyController.setPose({8.0f, 7.0f, 8.0f}, -135.0f, -30.0f);

  m_orbitController.setTarget({0.0f, 0.5f, 0.0f});
  m_orbitController.setOrbit(45.0f, 30.0f, 14.0f);

  m_topDownCamera.lookAt({0.0f, 15.0f, 0.01f}, {0.0f, 0.0f, 0.0f});
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

  m_botPosition.x = std::sin(m_time) * 2.0f;
  m_botPosition.y = 0.5f;
  m_botPosition.z = std::cos(m_time) * 2.0f;
}

void BotArenaGame::onRender(engine::Renderer& renderer, int width, int height) {
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

  renderer.setViewProjection(camera->viewProjection());

  engine::DebugRenderer debug(renderer.queue());

  debug.drawGrid(10.0f, 1.0f, {0.25f, 0.25f, 0.25f, 1.0f});

  debug.drawCube({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f},
                 {0.7f, 0.7f, 0.7f, 1.0f});
  debug.drawCube({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f},
                 {0.7f, 0.7f, 0.7f, 1.0f});
  debug.drawCube({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f},
                 {0.7f, 0.7f, 0.7f, 1.0f});
  debug.drawCube({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f},
                 {0.7f, 0.7f, 0.7f, 1.0f});

  debug.drawCube({2.0f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f},
                 {0.2f, 0.6f, 1.0f, 1.0f});
  debug.drawCube({-2.0f, 0.5f, -2.0f}, {1.5f, 1.0f, 1.5f},
                 {0.2f, 0.6f, 1.0f, 1.0f});

  debug.drawCube(m_botPosition, {0.5f, 1.0f, 0.5f}, {1.0f, 0.2f, 0.2f, 1.0f});

  debug.drawLine(m_botPosition, {0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f});
}

}  // namespace game
