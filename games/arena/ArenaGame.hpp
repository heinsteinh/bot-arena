#ifndef GAMES_ARENA_ARENAGAME_HPP
#define GAMES_ARENA_ARENAGAME_HPP

#include <entt/entt.hpp>
#include <random>
#include <vector>

#include "engine/core/Layer.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"
#include "engine/scene/Scene.hpp"
#include "engine/scene/SceneObject.hpp"

namespace arena {

class ArenaGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void spawnEntities();
  void stepSim(float dt);

  engine::Scene m_scene;
  engine::SceneObject m_camera;
  std::vector<engine::SceneObject> m_walls;
  engine::SceneObject m_ground;
  std::mt19937 m_rng{1337};
  float m_accumulator = 0.0f;
  bool m_resourcesReady = false;
  engine::MaterialHandle m_wallMat = 0;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_playerMat = 0;
  engine::MaterialHandle m_botMats[4] = {0, 0, 0, 0};

  static constexpr float kBotMaxSpeed = 2.5f;
  static constexpr float kBotMaxForce = 8.0f;

  static constexpr float kPlayerMaxHealth = 100.0f;
  static constexpr float kBotMaxHealth = 30.0f;
  static constexpr float kPlayerDps = 40.0f;
  static constexpr float kBotDps = 8.0f;
  static constexpr float kBotRegen = 6.0f;
  static constexpr float kFleeFraction = 0.35f;
  static constexpr float kArenaEdge = 4.5f;
  // Contact reach beyond the collision separation distance, so the ring of bots
  // held apart by collision still lands hits.
  static constexpr float kContactMargin = 0.2f;

  int m_kills = 0;
  int m_deaths = 0;
  engine::FontHandle m_font;
};

}  // namespace arena

#endif  // GAMES_ARENA_ARENAGAME_HPP
