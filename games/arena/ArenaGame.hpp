#ifndef GAMES_ARENA_ARENAGAME_HPP
#define GAMES_ARENA_ARENAGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/RenderCommand.hpp"

namespace arena {

class ArenaGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  engine::OrbitCameraController m_camera;
  bool m_resourcesReady = false;
  engine::MaterialHandle m_wallMat = 0;
  engine::MaterialHandle m_groundMat = 0;
};

}  // namespace arena

#endif  // GAMES_ARENA_ARENAGAME_HPP
