#include "games/shooter/ShooterGame.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "engine/ai/Steering.hpp"
#include "engine/assets/MeshBounds.hpp"
#include "engine/core/AssetPath.hpp"
#include "engine/core/Input.hpp"
#include "engine/gameplay/ShipControls.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "games/shooter/Components.hpp"

namespace shooter {

void ShooterGame::onAttach() {
  m_camera.setTarget({0.0f, 0.5f, 0.0f});
  m_camera.setOrbit(0.0f, 62.0f, 22.0f);

  const entt::entity player = m_registry.create();
  m_registry.emplace<Transform>(player, glm::vec3(0.0f, 0.4f, 0.0f), 0.6f,
                                0.0f);
  m_registry.emplace<Velocity>(player, glm::vec3(0.0f));
  m_registry.emplace<Player>(player);

  for (int i = 0; i < 150; ++i) stepSim(1.0f / 60.0f);
}

void ShooterGame::onUpdate(float dt) {
  m_camera.update(dt);
  m_accumulator += dt;
  const float step = 1.0f / 60.0f;
  int steps = 0;
  while (m_accumulator >= step && steps < 5) {
    stepSim(step);
    m_accumulator -= step;
    ++steps;
  }
}

void ShooterGame::spawnEnemy() {
  std::uniform_real_distribution<float> angleD(0.0f, 6.2831853f);
  std::uniform_int_distribution<int> tierD(0, 4);  // weighted toward grunts
  const int roll = tierD(m_rng);
  const int tier = roll < 3 ? 0 : (roll < 4 ? 1 : 2);
  const float a = angleD(m_rng);
  const entt::entity e = m_registry.create();
  m_registry.emplace<Transform>(
      e,
      glm::vec3(std::cos(a) * kSpawnRadius, 0.4f, std::sin(a) * kSpawnRadius),
      tier == 2 ? 0.8f : (tier == 1 ? 0.9f : 0.6f), 0.0f);
  m_registry.emplace<Velocity>(e, glm::vec3(0.0f));
  m_registry.emplace<Enemy>(e, tier);
}

void ShooterGame::stepSim(float dt) {
  // Input -> player velocity + facing.
  glm::vec3 dir(0.0f);
  if (engine::Input::isKeyDown(engine::Key::W)) dir.z -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::S)) dir.z += 1.0f;
  if (engine::Input::isKeyDown(engine::Key::A)) dir.x -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::D)) dir.x += 1.0f;

  glm::vec3 playerPos(0.0f);
  float playerYaw = 0.0f;
  for (const entt::entity e : m_registry.view<Transform, Velocity, Player>()) {
    Transform& t = m_registry.get<Transform>(e);
    Velocity& v = m_registry.get<Velocity>(e);
    if (glm::length(dir) > 0.01f) {
      v.value = glm::normalize(dir) * kPlayerSpeed;
      t.yaw = engine::headingToYaw(v.value);
    } else {
      v.value = glm::vec3(0.0f);
    }
    playerPos = t.position;
    playerYaw = t.yaw;
  }

  // Enemies seek the player.
  const float kTierSpeed[3] = {3.0f, 2.0f, 4.0f};
  for (const entt::entity e : m_registry.view<Transform, Velocity, Enemy>()) {
    Transform& t = m_registry.get<Transform>(e);
    Velocity& v = m_registry.get<Velocity>(e);
    const float maxSpeed = kTierSpeed[m_registry.get<Enemy>(e).tier];
    const glm::vec3 force =
        engine::seek(t.position, v.value, playerPos, maxSpeed, 8.0f);
    v.value += force * dt;
    v.value = engine::truncate(v.value, maxSpeed);
    if (glm::length(v.value) > 0.01f) t.yaw = engine::headingToYaw(v.value);
  }

  // Spawn on a timer up to the cap.
  m_spawnTimer -= dt;
  int enemyCount = 0;
  for (const entt::entity e : m_registry.view<Enemy>()) {
    (void)e;
    ++enemyCount;
  }
  if (m_spawnTimer <= 0.0f && enemyCount < kEnemyCap) {
    spawnEnemy();
    m_spawnTimer = kSpawnInterval;
  }

  // Auto-fire forward.
  m_fireTimer -= dt;
  if (m_fireTimer <= 0.0f) {
    const glm::vec3 fwd = engine::forwardFromYaw(playerYaw);
    const entt::entity b = m_registry.create();
    m_registry.emplace<Transform>(b, playerPos + fwd * kNoseOffset, 0.35f,
                                  playerYaw);
    m_registry.emplace<Velocity>(b, fwd * kBulletSpeed);
    m_registry.emplace<Bullet>(b, kBulletLife);
    m_fireTimer = kFireCooldown;
  }

  // Integrate everything; age + cull bullets.
  for (const entt::entity e : m_registry.view<Transform, Velocity>()) {
    m_registry.get<Transform>(e).position +=
        m_registry.get<Velocity>(e).value * dt;
  }
  std::vector<entt::entity> deadBullets;
  for (const entt::entity e : m_registry.view<Transform, Bullet>()) {
    Bullet& bu = m_registry.get<Bullet>(e);
    bu.life -= dt;
    const glm::vec3& p = m_registry.get<Transform>(e).position;
    if (bu.life <= 0.0f || std::abs(p.x) > kCullBound ||
        std::abs(p.z) > kCullBound) {
      deadBullets.push_back(e);
    }
  }
  for (const entt::entity e : deadBullets) m_registry.destroy(e);
}

void ShooterGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    engine::ResourceRegistry& reg = renderer.registry();
    m_groundMat =
        reg.registerMaterial({{0.05f, 0.06f, 0.09f, 1.0f}, 0.0f, 0.9f, s});
    m_playerModel = engine::loadModel(
        engine::assetPath("asteroid-game/Ships/Viper.obj"), reg, s);
    m_enemyModels[0] =
        engine::loadModel(engine::assetPath("SpaceGame/Spaceship.obj"), reg, s);
    m_enemyModels[1] = engine::loadModel(
        engine::assetPath("SpaceGame/Spaceship3.obj"), reg, s);
    m_enemyModels[2] = engine::loadModel(
        engine::assetPath("asteroid-game/Ships/eliteship.obj"), reg, s);
    m_bulletModel = engine::loadModel(
        engine::assetPath("asteroid-game/Objects/Projectile.obj"), reg, s);
    m_font = engine::Font::Load(
        std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf", 32);
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_camera.resize(aspect);
  renderer.setCamera(m_camera.camera());

  std::vector<engine::PointLight> lights;
  engine::PointLight key;
  key.positionRadius = glm::vec4(0.0f, 8.0f, 4.0f, 40.0f);
  key.color = glm::vec4(1.0f, 0.97f, 0.9f, 3.0f);
  lights.push_back(key);
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(),
                              m_camera.camera());

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.2f, 0.0f});
  ground = glm::scale(ground, {28.0f, 0.05f, 28.0f});
  meshes.submit(cube, m_groundMat, ground);

  const auto drawModel = [&](const engine::Model& model, const glm::vec3& pos,
                             float yaw, float scale) {
    if (!model.valid) return;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
    m = glm::rotate(m, yaw, {0.0f, 1.0f, 0.0f});
    m = glm::scale(m, glm::vec3(scale));
    m = m * engine::fitToUnitTransform(model.bounds);
    for (const engine::Submesh& sm : model.submeshes) {
      meshes.submit(sm.mesh, sm.material, m);
    }
  };

  for (const entt::entity e : m_registry.view<Transform, Player>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_playerModel, t.position, t.yaw, t.scale);
  }
  for (const entt::entity e : m_registry.view<Transform, Enemy>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_enemyModels[m_registry.get<Enemy>(e).tier], t.position, t.yaw,
              t.scale);
  }
  for (const entt::entity e : m_registry.view<Transform, Bullet>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_bulletModel, t.position, t.yaw, t.scale);
  }
}

}  // namespace shooter
