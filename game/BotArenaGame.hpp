#ifndef GAME_BOTARENAGAME_HPP
#define GAME_BOTARENAGAME_HPP

#include <cstddef>
#include <glm/glm.hpp>

#include "engine/core/Layer.hpp"
#include "engine/renderer/Camera2D.hpp"
#include "engine/renderer/FlyCameraController.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/OrthographicCamera.hpp"
#include "engine/renderer/RenderCommand.hpp"

namespace game {

class BotArenaGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onDetach() override;

  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  enum class CameraMode { Fly, Orbit, TopDown };

  engine::FlyCameraController m_flyController;
  engine::OrbitCameraController m_orbitController;
  engine::OrthographicCamera m_topDownCamera;
  engine::OrthographicCamera m_minimapCam;
  engine::Camera2D m_uiCamera;

  CameraMode m_cameraMode = CameraMode::Fly;

  float m_time = 0.0f;

  bool m_resourcesReady = false;
  engine::MaterialHandle m_wallMat = 0;
  engine::MaterialHandle m_swarmMats[3] = {0, 0, 0};

  static constexpr std::size_t kBotCount = 2048;

  void cycleCameraMode();
};

}  // namespace game

#endif  // GAME_BOTARENAGAME_HPP
