#ifndef GAMES_ARENA_ARENAGAME_HPP
#define GAMES_ARENA_ARENAGAME_HPP

#include <entt/entt.hpp>
#include <random>

#include "engine/core/Layer.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/RenderCommand.hpp"

namespace arena {

class ArenaGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void spawnEntities();
  void stepSim(float dt);

  engine::OrbitCameraController m_camera;
  entt::registry m_registry;
  std::mt19937 m_rng{1337};
  float m_accumulator = 0.0f;
  bool m_resourcesReady = false;
  engine::MaterialHandle m_wallMat = 0;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_playerMat = 0;
  engine::MaterialHandle m_botMats[4] = {0, 0, 0, 0};
};

}  // namespace arena

#endif  // GAMES_ARENA_ARENAGAME_HPP
