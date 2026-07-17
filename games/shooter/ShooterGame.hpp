#ifndef GAMES_SHOOTER_SHOOTERGAME_HPP
#define GAMES_SHOOTER_SHOOTERGAME_HPP

#include <entt/entt.hpp>
#include <random>
#include <vector>

#include "engine/assets/ModelLoader.hpp"
#include "engine/core/Layer.hpp"
#include "engine/particles/ParticleSystem.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"
#include "engine/scene/Scene.hpp"
#include "engine/scene/SceneObject.hpp"

namespace shooter {

class ShooterGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void stepSim(float dt);
  void spawnEnemy();
  void destroyActor(entt::entity e);

  // Ships/enemies/bullets spawn during the onAttach warm-up (before models are
  // registered in onRender), so their ModelComponent is attached lazily: this
  // adds one to any actor still missing it, once resources are ready.
  void attachMissingModels();

  engine::Scene m_scene;
  engine::SceneObject m_camera;
  engine::SceneObject m_ground;
  std::mt19937 m_rng{2025};
  float m_accumulator = 0.0f;
  float m_fireTimer = 0.0f;
  float m_spawnTimer = 0.0f;
  int m_score = 0;
  int m_lives = 3;

  bool m_resourcesReady = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::ModelHandle m_playerModel = 0;
  engine::ModelHandle m_enemyModels[3] = {0, 0, 0};
  engine::ModelHandle m_bulletModel = 0;
  engine::MaterialHandle m_playerBulletMat = 0;
  engine::MaterialHandle m_enemyBulletMat = 0;
  engine::FontHandle m_font;
  engine::ParticleSystem m_explosions;

  static constexpr float kPlayerSpeed = 6.0f;
  static constexpr float kBulletSpeed = 16.0f;
  static constexpr float kBulletLife = 1.4f;
  static constexpr float kFireCooldown = 0.16f;
  static constexpr float kNoseOffset = 0.7f;
  static constexpr float kBulletRadius = 0.2f;
  static constexpr float kPlayerRadius = 0.6f;
  static constexpr float kEnemyRadius = 0.7f;
  static constexpr float kSpawnInterval = 1.1f;
  static constexpr float kSpawnRadius = 9.0f;
  static constexpr float kCullBound = 13.0f;
  static constexpr int kEnemyCap = 12;
  static constexpr float kMaxHealth = 100.0f;
  static constexpr float kEnemyBulletSpeed = 9.0f;
  static constexpr float kEnemyBulletLife = 2.5f;
  static constexpr float kFireRange = 11.0f;
  static constexpr float kEnemyFireCooldown = 1.6f;
  static constexpr float kBulletYawOffset = 0.0f;
  static constexpr float kEnemyBulletDamage = 10.0f;
  static constexpr float kRamDamage = 25.0f;
  static constexpr int kLives = 3;
  // The ship models point opposite their heading, so render them rotated
  // 180 degrees relative to the game's facing yaw.
  static constexpr float kShipYaw = 3.1415927f;
};

}  // namespace shooter

#endif  // GAMES_SHOOTER_SHOOTERGAME_HPP
