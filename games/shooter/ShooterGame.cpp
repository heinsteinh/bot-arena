#include "games/shooter/ShooterGame.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "engine/ai/Steering.hpp"
#include "engine/assets/MeshBounds.hpp"
#include "engine/core/AssetPath.hpp"
#include "engine/core/Input.hpp"
#include "engine/gameplay/Combat.hpp"
#include "engine/gameplay/ShipControls.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/ParticleInstance.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "games/shooter/Components.hpp"

namespace {

engine::EmitParams enemyExplosion(int tier) {
  engine::EmitParams p;
  p.count = 40 + tier * 20;  // 40 / 60 / 80
  p.speedMin = 2.0f;
  p.speedMax = 4.5f + tier * 1.5f;
  p.direction = {0.0f, 0.0f, 0.0f};  // radial
  p.spread = 1.0f;
  p.color = {1.0f, 0.5f, 0.15f};  // warm orange
  p.sizeMin = 0.18f;
  p.sizeMax = 0.32f + tier * 0.1f;
  p.lifeMin = 0.7f;
  p.lifeMax = 1.6f;
  p.gravity = {0.0f, -5.0f, 0.0f};
  return p;
}

engine::EmitParams playerExplosion() {
  engine::EmitParams p;
  p.count = 100;
  p.speedMin = 3.0f;
  p.speedMax = 9.0f;
  p.direction = {0.0f, 0.0f, 0.0f};
  p.spread = 1.0f;
  p.color = {1.0f, 0.85f, 0.4f};  // bright gold-white
  p.sizeMin = 0.2f;
  p.sizeMax = 0.5f;
  p.lifeMin = 0.8f;
  p.lifeMax = 1.6f;
  p.gravity = {0.0f, -4.0f, 0.0f};
  return p;
}

}  // namespace

namespace shooter {

void ShooterGame::onAttach() {
  m_camera.setTarget({0.0f, 0.5f, 0.0f});
  m_camera.setOrbit(0.0f, 62.0f, 22.0f);

  const entt::entity player = m_registry.create();
  m_registry.emplace<Transform>(player, glm::vec3(0.0f, 0.4f, 0.0f), 1.1f,
                                0.0f);
  m_registry.emplace<Velocity>(player, glm::vec3(0.0f));
  m_registry.emplace<Player>(player);
  m_registry.emplace<Health>(player, kMaxHealth, kMaxHealth);

  for (int i = 0; i < 250; ++i) stepSim(1.0f / 60.0f);
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
      tier == 2 ? 1.5f : (tier == 1 ? 1.3f : 0.9f), 0.0f);
  m_registry.emplace<Velocity>(e, glm::vec3(0.0f));
  std::uniform_real_distribution<float> ftD(0.3f, kEnemyFireCooldown);
  m_registry.emplace<Enemy>(e, tier, ftD(m_rng));
  const float hp = tier == 2 ? 5.0f : (tier == 1 ? 3.0f : 1.0f);
  m_registry.emplace<Health>(e, hp, hp);
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
      // Idle: hold position and auto-aim at the nearest enemy.
      v.value = glm::vec3(0.0f);
      float best = 1e18f;
      glm::vec3 target(0.0f);
      bool found = false;
      for (const entt::entity en : m_registry.view<Transform, Enemy>()) {
        const glm::vec3& ep = m_registry.get<Transform>(en).position;
        const float dsq = glm::dot(ep - t.position, ep - t.position);
        if (dsq < best) {
          best = dsq;
          target = ep;
          found = true;
        }
      }
      if (found) t.yaw = engine::headingToYaw(target - t.position);
    }
    playerPos = t.position;
    playerYaw = t.yaw;
  }

  // Enemies seek the player, face the player, and fire when in range.
  const float kTierSpeed[3] = {3.0f, 2.0f, 4.0f};
  std::vector<glm::vec3> enemyShots;
  for (const entt::entity e : m_registry.view<Transform, Velocity, Enemy>()) {
    Transform& t = m_registry.get<Transform>(e);
    Velocity& v = m_registry.get<Velocity>(e);
    Enemy& en = m_registry.get<Enemy>(e);
    const float maxSpeed = kTierSpeed[en.tier];
    const glm::vec3 force =
        engine::seek(t.position, v.value, playerPos, maxSpeed, 8.0f);
    v.value += force * dt;
    v.value = engine::truncate(v.value, maxSpeed);
    t.yaw = engine::headingToYaw(playerPos - t.position);
    en.fireTimer -= dt;
    if (en.fireTimer <= 0.0f) {
      if (glm::length(playerPos - t.position) < kFireRange) {
        enemyShots.push_back(t.position);
        en.fireTimer = kEnemyFireCooldown;
      } else {
        en.fireTimer = 0.0f;  // ready to fire the moment it is in range
      }
    }
  }
  for (const glm::vec3& origin : enemyShots) {
    const glm::vec3 aim = glm::normalize(playerPos - origin);
    const entt::entity eb = m_registry.create();
    m_registry.emplace<Transform>(eb, origin + aim * 0.8f, 1.3f,
                                  engine::headingToYaw(aim));
    m_registry.emplace<Velocity>(eb, aim * kEnemyBulletSpeed);
    m_registry.emplace<Bullet>(eb, kEnemyBulletLife, false);
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
    m_registry.emplace<Transform>(b, playerPos + fwd * kNoseOffset, 1.3f,
                                  engine::headingToYaw(fwd));
    m_registry.emplace<Velocity>(b, fwd * kBulletSpeed);
    m_registry.emplace<Bullet>(b, kBulletLife, true);
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

  // Player bullets damage enemies; enemies die when their health is spent.
  const int kWeight[3] = {1, 2, 3};
  std::vector<entt::entity> killed;
  for (const entt::entity b : m_registry.view<Transform, Bullet>()) {
    if (!m_registry.get<Bullet>(b).fromPlayer) continue;
    const glm::vec3& bp = m_registry.get<Transform>(b).position;
    for (const entt::entity en : m_registry.view<Transform, Enemy, Health>()) {
      const glm::vec3& ep = m_registry.get<Transform>(en).position;
      if (engine::circlesOverlapXZ(bp, kBulletRadius, ep, kEnemyRadius)) {
        Health& eh = m_registry.get<Health>(en);
        if (eh.current <= 0.0f) continue;  // already dying this frame
        eh.current = engine::adjustHealth(eh.current, -1.0f, eh.max);
        killed.push_back(b);
        if (eh.current <= 0.0f) {
          const int tier = m_registry.get<Enemy>(en).tier;
          m_explosions.emit(enemyExplosion(tier), ep, m_rng);
          m_score += kWeight[tier];
          killed.push_back(en);
        }
        break;
      }
    }
  }
  for (const entt::entity e : killed) {
    if (m_registry.valid(e)) m_registry.destroy(e);
  }

  // Enemy bullets and rams damage the player.
  entt::entity playerEnt = entt::null;
  for (const entt::entity e : m_registry.view<Player>()) playerEnt = e;
  Health& ph = m_registry.get<Health>(playerEnt);

  std::vector<entt::entity> spent;
  for (const entt::entity b : m_registry.view<Transform, Bullet>()) {
    if (m_registry.get<Bullet>(b).fromPlayer) continue;
    const glm::vec3& bp = m_registry.get<Transform>(b).position;
    if (engine::circlesOverlapXZ(bp, kBulletRadius, playerPos, kPlayerRadius)) {
      ph.current =
          engine::adjustHealth(ph.current, -kEnemyBulletDamage, ph.max);
      spent.push_back(b);
    }
  }
  for (const entt::entity e : spent) m_registry.destroy(e);

  std::vector<entt::entity> rammed;
  for (const entt::entity en : m_registry.view<Transform, Enemy>()) {
    const glm::vec3& ep = m_registry.get<Transform>(en).position;
    if (engine::circlesOverlapXZ(ep, kEnemyRadius, playerPos, kPlayerRadius)) {
      m_explosions.emit(enemyExplosion(m_registry.get<Enemy>(en).tier), ep,
                        m_rng);
      ph.current = engine::adjustHealth(ph.current, -kRamDamage, ph.max);
      rammed.push_back(en);
    }
  }
  for (const entt::entity e : rammed) m_registry.destroy(e);

  // Death: lose a life and respawn; out of lives resets the run.
  if (ph.current <= 0.0f) {
    m_explosions.emit(playerExplosion(), playerPos, m_rng);
    --m_lives;
    m_registry.get<Transform>(playerEnt).position = {0.0f, 0.4f, 0.0f};
    ph.current = ph.max;
    if (m_lives > 0) {
      std::vector<entt::entity> clearBullets;
      for (const entt::entity b : m_registry.view<Bullet>()) {
        if (!m_registry.get<Bullet>(b).fromPlayer) clearBullets.push_back(b);
      }
      for (const entt::entity e : clearBullets) m_registry.destroy(e);
    } else {
      std::vector<entt::entity> wipe;
      for (const entt::entity e : m_registry.view<Enemy>()) wipe.push_back(e);
      for (const entt::entity e : m_registry.view<Bullet>()) wipe.push_back(e);
      for (const entt::entity e : wipe) m_registry.destroy(e);
      m_score = 0;
      m_lives = kLives;
    }
  }

  m_explosions.update(dt);
}

void ShooterGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    engine::ResourceRegistry& reg = renderer.registry();
    m_groundMat =
        reg.registerMaterial({{0.05f, 0.06f, 0.09f, 1.0f}, 0.0f, 0.9f, s});
    m_playerBulletMat =
        reg.registerMaterial({{0.3f, 1.0f, 0.4f, 1.0f}, 0.2f, 0.3f, s});
    m_enemyBulletMat =
        reg.registerMaterial({{1.0f, 0.3f, 0.2f, 1.0f}, 0.2f, 0.3f, s});
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

  // The ship models point opposite the heading, so rotate them 180 deg.
  constexpr float kShipYaw = 3.1415927f;
  for (const entt::entity e : m_registry.view<Transform, Player>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_playerModel, t.position, t.yaw + kShipYaw, t.scale);
  }
  for (const entt::entity e : m_registry.view<Transform, Enemy>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_enemyModels[m_registry.get<Enemy>(e).tier], t.position,
              t.yaw + kShipYaw, t.scale);
  }
  for (const entt::entity e : m_registry.view<Transform, Bullet>()) {
    const Transform& t = m_registry.get<Transform>(e);
    const engine::MaterialHandle mat = m_registry.get<Bullet>(e).fromPlayer
                                           ? m_playerBulletMat
                                           : m_enemyBulletMat;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), t.position);
    m = glm::rotate(m, t.yaw + kBulletYawOffset, {0.0f, 1.0f, 0.0f});
    m = glm::scale(m, glm::vec3(t.scale));
    m = m * engine::fitToUnitTransform(m_bulletModel.bounds);
    for (const engine::Submesh& sm : m_bulletModel.submeshes) {
      meshes.submit(sm.mesh, mat, m);
    }
  }

  std::vector<engine::ParticleInstance> particles;
  for (const engine::Particle& p : m_explosions.particles()) {
    particles.push_back(
        {p.position, p.size, glm::vec4(engine::renderColor(p), 1.0f)});
  }
  renderer.submitParticles(particles);

  if (m_font) {
    int enemies = 0;
    for (const entt::entity e : m_registry.view<Enemy>()) {
      (void)e;
      ++enemies;
    }
    float hp = 0.0f, hpMax = 0.0f;
    for (const entt::entity e : m_registry.view<Health, Player>()) {
      hp = m_registry.get<Health>(e).current;
      hpMax = m_registry.get<Health>(e).max;
    }
    const float bottom = static_cast<float>(height);
    renderer.drawText(*m_font, "Score: " + std::to_string(m_score), 8.0f,
                      bottom - 78.0f, 0.7f, glm::vec4(1.0f));
    renderer.drawText(*m_font,
                      "HP: " + std::to_string(static_cast<int>(hp)) + " / " +
                          std::to_string(static_cast<int>(hpMax)),
                      8.0f, bottom - 56.0f, 0.7f, glm::vec4(1.0f));
    renderer.drawText(*m_font, "Lives: " + std::to_string(m_lives), 8.0f,
                      bottom - 34.0f, 0.7f, glm::vec4(1.0f));
    renderer.drawText(*m_font, "Enemies: " + std::to_string(enemies), 8.0f,
                      bottom - 12.0f, 0.7f, glm::vec4(1.0f));
  }
}

}  // namespace shooter
