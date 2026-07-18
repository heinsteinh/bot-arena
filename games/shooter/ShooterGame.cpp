#include "games/shooter/ShooterGame.hpp"

#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "engine/ai/Steering.hpp"
#include "engine/assets/ModelLoader.hpp"
#include "engine/core/AssetPath.hpp"
#include "engine/core/Input.hpp"
#include "engine/gameplay/Combat.hpp"
#include "engine/gameplay/ShipControls.hpp"
#include "engine/renderer/ParticleInstance.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ControllerComponents.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/ModelComponent.hpp"
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
  m_camera = m_scene.createObject("Camera");
  engine::CameraComponent& cam =
      m_camera.addComponent<engine::CameraComponent>();
  // Match the old OrbitCameraController's perspective (60 deg FOV, 0.1/100
  // clip) so the framing is unchanged.
  cam.fov = 60.0f;
  cam.perspNear = 0.1f;
  cam.perspFar = 100.0f;
  engine::OrbitControllerComponent& oc =
      m_camera.addComponent<engine::OrbitControllerComponent>();
  oc.targetPoint = {0.0f, 0.5f, 0.0f};
  // OrbitControllerComponent measures yaw from +Z (FPS-style), while the old
  // OrbitCameraController measured it from +X -- a 90 degree offset. The old
  // controller's setOrbit(0, ...) needs yaw=90 here to land the camera at
  // the same position (arena's yaw=45 masked this, since swapping the axes
  // at 45 degrees is a no-op).
  oc.yaw = 90.0f;
  oc.pitch = 62.0f;  // elevation above target, same convention as before
  oc.distance = 22.0f;
  oc.maxDistance = 40.0f;

  m_ground = m_scene.createObject("Ground");
  engine::TransformComponent& gt =
      m_ground.getComponent<engine::TransformComponent>();
  gt.translation = {0.0f, -0.2f, 0.0f};
  gt.scale = {28.0f, 0.05f, 28.0f};

  engine::SceneObject player = m_scene.createObject("Player");
  engine::TransformComponent& pt =
      player.getComponent<engine::TransformComponent>();
  pt.translation = {0.0f, 0.4f, 0.0f};
  pt.scale = glm::vec3(1.1f);
  player.addComponent<Velocity>(glm::vec3(0.0f));
  // Player is an empty tag type; entt's emplace() returns void (not T&) for
  // those, so it can't go through SceneObject::addComponent<T>'s T&
  // signature -- emplace it on the registry directly.
  m_scene.registry().emplace<Player>(static_cast<entt::entity>(player));
  player.addComponent<Health>(Health{kMaxHealth, kMaxHealth});

  // Static point key light, previously rebuilt every frame in onRender.
  engine::SceneObject keyLight = m_scene.createObject("KeyLight");
  keyLight.getComponent<engine::TransformComponent>().translation = {0.0f, 8.0f,
                                                                     4.0f};
  keyLight.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Point, glm::vec3(1.0f, 0.97f, 0.9f), 3.0f, 40.0f});

  for (int i = 0; i < 250; ++i) stepSim(1.0f / 60.0f);
}

void ShooterGame::onUpdate(float dt) {
  m_scene.update(dt);  // drives the camera's OrbitControllerComponent
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

  engine::SceneObject e = m_scene.createObject("Enemy");
  engine::TransformComponent& t = e.getComponent<engine::TransformComponent>();
  t.translation = {std::cos(a) * kSpawnRadius, 0.4f,
                   std::sin(a) * kSpawnRadius};
  t.scale = glm::vec3(tier == 2 ? 1.5f : (tier == 1 ? 1.3f : 0.9f));
  e.addComponent<Velocity>(glm::vec3(0.0f));
  std::uniform_real_distribution<float> ftD(0.3f, kEnemyFireCooldown);
  e.addComponent<Enemy>(Enemy{tier, ftD(m_rng)});
  const float hp = tier == 2 ? 5.0f : (tier == 1 ? 3.0f : 1.0f);
  e.addComponent<Health>(Health{hp, hp});
}

void ShooterGame::destroyActor(entt::entity e) {
  entt::registry& reg = m_scene.registry();
  if (reg.valid(e)) reg.destroy(e);  // ModelComponent is destroyed with it
}

void ShooterGame::stepSim(float dt) {
  entt::registry& reg = m_scene.registry();

  // Input -> player velocity + facing.
  glm::vec3 dir(0.0f);
  if (engine::Input::isKeyDown(engine::Key::W)) dir.z -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::S)) dir.z += 1.0f;
  if (engine::Input::isKeyDown(engine::Key::A)) dir.x -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::D)) dir.x += 1.0f;

  glm::vec3 playerPos(0.0f);
  glm::quat playerRot(1.0f, 0.0f, 0.0f, 0.0f);
  for (const entt::entity e :
       reg.view<engine::TransformComponent, Velocity, Player>()) {
    engine::TransformComponent& t = reg.get<engine::TransformComponent>(e);
    Velocity& v = reg.get<Velocity>(e);
    if (glm::length(dir) > 0.01f) {
      v.value = glm::normalize(dir) * kPlayerSpeed;
      t.rotation = glm::angleAxis(engine::headingToYaw(v.value),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
      // Idle: hold position and auto-aim at the nearest enemy.
      v.value = glm::vec3(0.0f);
      float best = 1e18f;
      glm::vec3 target(0.0f);
      bool found = false;
      for (const entt::entity en :
           reg.view<engine::TransformComponent, Enemy>()) {
        const glm::vec3& ep =
            reg.get<engine::TransformComponent>(en).translation;
        const float dsq = glm::dot(ep - t.translation, ep - t.translation);
        if (dsq < best) {
          best = dsq;
          target = ep;
          found = true;
        }
      }
      if (found) {
        t.rotation =
            glm::angleAxis(engine::headingToYaw(target - t.translation),
                           glm::vec3(0.0f, 1.0f, 0.0f));
      }
    }
    playerPos = t.translation;
    playerRot = t.rotation;
  }

  // Enemies seek the player, face the player, and fire when in range.
  const float kTierSpeed[3] = {3.0f, 2.0f, 4.0f};
  std::vector<glm::vec3> enemyShots;
  for (const entt::entity e :
       reg.view<engine::TransformComponent, Velocity, Enemy>()) {
    engine::TransformComponent& t = reg.get<engine::TransformComponent>(e);
    Velocity& v = reg.get<Velocity>(e);
    Enemy& en = reg.get<Enemy>(e);
    const float maxSpeed = kTierSpeed[en.tier];
    const glm::vec3 force =
        engine::seek(t.translation, v.value, playerPos, maxSpeed, 8.0f);
    v.value += force * dt;
    v.value = engine::truncate(v.value, maxSpeed);
    t.rotation = glm::angleAxis(engine::headingToYaw(playerPos - t.translation),
                                glm::vec3(0.0f, 1.0f, 0.0f));
    en.fireTimer -= dt;
    if (en.fireTimer <= 0.0f) {
      if (glm::length(playerPos - t.translation) < kFireRange) {
        enemyShots.push_back(t.translation);
        en.fireTimer = kEnemyFireCooldown;
      } else {
        en.fireTimer = 0.0f;  // ready to fire the moment it is in range
      }
    }
  }
  for (const glm::vec3& origin : enemyShots) {
    const glm::vec3 aim = glm::normalize(playerPos - origin);
    engine::SceneObject eb = m_scene.createObject("EnemyBullet");
    engine::TransformComponent& ebt =
        eb.getComponent<engine::TransformComponent>();
    ebt.translation = origin + aim * 0.8f;
    ebt.scale = glm::vec3(1.3f);
    ebt.rotation =
        glm::angleAxis(engine::headingToYaw(aim), glm::vec3(0.0f, 1.0f, 0.0f));
    eb.addComponent<Velocity>(aim * kEnemyBulletSpeed);
    eb.addComponent<Bullet>(Bullet{kEnemyBulletLife, false});
  }

  // Spawn on a timer up to the cap.
  m_spawnTimer -= dt;
  int enemyCount = 0;
  for (const entt::entity e : reg.view<Enemy>()) {
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
    const glm::vec3 fwd = playerRot * glm::vec3(0.0f, 0.0f, 1.0f);
    engine::SceneObject b = m_scene.createObject("PlayerBullet");
    engine::TransformComponent& bt =
        b.getComponent<engine::TransformComponent>();
    bt.translation = playerPos + fwd * kNoseOffset;
    bt.scale = glm::vec3(1.3f);
    bt.rotation = playerRot;
    b.addComponent<Velocity>(fwd * kBulletSpeed);
    b.addComponent<Bullet>(Bullet{kBulletLife, true});
    m_fireTimer = kFireCooldown;
  }

  // Integrate everything; age + cull bullets.
  for (const entt::entity e :
       reg.view<engine::TransformComponent, Velocity>()) {
    reg.get<engine::TransformComponent>(e).translation +=
        reg.get<Velocity>(e).value * dt;
  }
  std::vector<entt::entity> deadBullets;
  for (const entt::entity e : reg.view<engine::TransformComponent, Bullet>()) {
    Bullet& bu = reg.get<Bullet>(e);
    bu.life -= dt;
    const glm::vec3& p = reg.get<engine::TransformComponent>(e).translation;
    if (bu.life <= 0.0f || std::abs(p.x) > kCullBound ||
        std::abs(p.z) > kCullBound) {
      deadBullets.push_back(e);
    }
  }
  for (const entt::entity e : deadBullets) destroyActor(e);

  // Player bullets damage enemies; enemies die when their health is spent.
  const int kWeight[3] = {1, 2, 3};
  std::vector<entt::entity> killed;
  for (const entt::entity b : reg.view<engine::TransformComponent, Bullet>()) {
    if (!reg.get<Bullet>(b).fromPlayer) continue;
    const glm::vec3& bp = reg.get<engine::TransformComponent>(b).translation;
    for (const entt::entity en :
         reg.view<engine::TransformComponent, Enemy, Health>()) {
      const glm::vec3& ep = reg.get<engine::TransformComponent>(en).translation;
      if (engine::circlesOverlapXZ(bp, kBulletRadius, ep, kEnemyRadius)) {
        Health& eh = reg.get<Health>(en);
        if (eh.current <= 0.0f) continue;  // already dying this frame
        eh.current = engine::adjustHealth(eh.current, -1.0f, eh.max);
        killed.push_back(b);
        if (eh.current <= 0.0f) {
          const int tier = reg.get<Enemy>(en).tier;
          m_explosions.emit(enemyExplosion(tier), ep, m_rng);
          m_score += kWeight[tier];
          killed.push_back(en);
        }
        break;
      }
    }
  }
  for (const entt::entity e : killed) destroyActor(e);

  // Enemy bullets and rams damage the player.
  entt::entity playerEnt = entt::null;
  for (const entt::entity e : reg.view<Player>()) playerEnt = e;
  Health& ph = reg.get<Health>(playerEnt);

  std::vector<entt::entity> spent;
  for (const entt::entity b : reg.view<engine::TransformComponent, Bullet>()) {
    if (reg.get<Bullet>(b).fromPlayer) continue;
    const glm::vec3& bp = reg.get<engine::TransformComponent>(b).translation;
    if (engine::circlesOverlapXZ(bp, kBulletRadius, playerPos, kPlayerRadius)) {
      ph.current =
          engine::adjustHealth(ph.current, -kEnemyBulletDamage, ph.max);
      spent.push_back(b);
    }
  }
  for (const entt::entity e : spent) destroyActor(e);

  std::vector<entt::entity> rammed;
  for (const entt::entity en : reg.view<engine::TransformComponent, Enemy>()) {
    const glm::vec3& ep = reg.get<engine::TransformComponent>(en).translation;
    if (engine::circlesOverlapXZ(ep, kEnemyRadius, playerPos, kPlayerRadius)) {
      m_explosions.emit(enemyExplosion(reg.get<Enemy>(en).tier), ep, m_rng);
      ph.current = engine::adjustHealth(ph.current, -kRamDamage, ph.max);
      rammed.push_back(en);
    }
  }
  for (const entt::entity e : rammed) destroyActor(e);

  // Death: lose a life and respawn; out of lives resets the run.
  if (ph.current <= 0.0f) {
    m_explosions.emit(playerExplosion(), playerPos, m_rng);
    --m_lives;
    reg.get<engine::TransformComponent>(playerEnt).translation = {0.0f, 0.4f,
                                                                  0.0f};
    ph.current = ph.max;
    if (m_lives > 0) {
      std::vector<entt::entity> clearBullets;
      for (const entt::entity b : reg.view<Bullet>()) {
        if (!reg.get<Bullet>(b).fromPlayer) clearBullets.push_back(b);
      }
      for (const entt::entity e : clearBullets) destroyActor(e);
    } else {
      std::vector<entt::entity> wipe;
      for (const entt::entity e : reg.view<Enemy>()) wipe.push_back(e);
      for (const entt::entity e : reg.view<Bullet>()) wipe.push_back(e);
      for (const entt::entity e : wipe) destroyActor(e);
      m_score = 0;
      m_lives = kLives;
    }
  }

  m_explosions.update(dt);
}

void ShooterGame::attachMissingModels() {
  entt::registry& reg = m_scene.registry();
  const glm::quat shipFacing =
      glm::angleAxis(kShipYaw, glm::vec3(0.0f, 1.0f, 0.0f));

  for (const entt::entity e : reg.view<Player>()) {
    if (!reg.all_of<engine::ModelComponent>(e)) {
      reg.emplace<engine::ModelComponent>(
          e, engine::ModelComponent{m_playerModel, true, 0, shipFacing});
    }
  }
  for (const entt::entity e : reg.view<Enemy>()) {
    if (!reg.all_of<engine::ModelComponent>(e)) {
      reg.emplace<engine::ModelComponent>(
          e, engine::ModelComponent{m_enemyModels[reg.get<Enemy>(e).tier], true,
                                    0, shipFacing});
    }
  }
  for (const entt::entity e : reg.view<Bullet>()) {
    if (!reg.all_of<engine::ModelComponent>(e)) {
      const engine::MaterialHandle tint =
          reg.get<Bullet>(e).fromPlayer ? m_playerBulletMat : m_enemyBulletMat;
      reg.emplace<engine::ModelComponent>(
          e, engine::ModelComponent{m_bulletModel, true, tint,
                                    glm::quat(1.0f, 0.0f, 0.0f, 0.0f)});
    }
  }
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
    m_playerModel = reg.registerModel(engine::loadModel(
        engine::assetPath("asteroid-game/Ships/Viper.obj"), reg, s));
    m_enemyModels[0] = reg.registerModel(engine::loadModel(
        engine::assetPath("SpaceGame/Spaceship.obj"), reg, s));
    m_enemyModels[1] = reg.registerModel(engine::loadModel(
        engine::assetPath("SpaceGame/Spaceship3.obj"), reg, s));
    m_enemyModels[2] = reg.registerModel(engine::loadModel(
        engine::assetPath("asteroid-game/Ships/eliteship.obj"), reg, s));
    m_bulletModel = reg.registerModel(engine::loadModel(
        engine::assetPath("asteroid-game/Objects/Projectile.obj"), reg, s));

    // The ground never moved after onAttach's warm-up spawned it (before
    // the Renderer existed), so attach its MeshComponent now, on the first
    // onRender, once the cube handle + material are available.
    const engine::MeshHandle cube = renderer.unitCubeMesh();
    m_ground.addComponent<engine::MeshComponent>(
        engine::MeshComponent{cube, m_groundMat});

    m_resourcesReady = true;
  }

  if (!m_font) {
    engine::FontDesc desc;
    desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf";
    desc.pixelSize = 32;
    m_font = renderer.fonts().load(desc);
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;

  // Ships/enemies/bullets spawn continuously (before resources are ready
  // during onAttach's warm-up, and every frame afterward), so backfill
  // any actor still missing its ModelComponent right before drawing.
  attachMissingModels();

  m_scene.render(renderer, aspect);

  std::vector<engine::ParticleInstance> particles;
  for (const engine::Particle& p : m_explosions.particles()) {
    particles.push_back(
        {p.position, p.size, glm::vec4(engine::renderColor(p), 1.0f)});
  }
  renderer.submitParticles(particles);

  if (m_font) {
    entt::registry& reg = m_scene.registry();
    int enemies = 0;
    for (const entt::entity e : reg.view<Enemy>()) {
      (void)e;
      ++enemies;
    }
    float hp = 0.0f, hpMax = 0.0f;
    for (const entt::entity e : reg.view<Health, Player>()) {
      hp = reg.get<Health>(e).current;
      hpMax = reg.get<Health>(e).max;
    }
    const float bottom = static_cast<float>(height);

    engine::TextPlacement scoreP;
    scoreP.pos = {8.0f, bottom - 78.0f};
    scoreP.scale = 0.7f;
    engine::TextStyle scoreS;
    scoreS.fillColor = glm::vec4(1.0f);
    renderer.drawText(m_font, "Score: " + std::to_string(m_score), scoreP,
                      scoreS);

    engine::TextPlacement hpP;
    hpP.pos = {8.0f, bottom - 56.0f};
    hpP.scale = 0.7f;
    engine::TextStyle hpS;
    hpS.fillColor = glm::vec4(1.0f);
    renderer.drawText(m_font,
                      "HP: " + std::to_string(static_cast<int>(hp)) + " / " +
                          std::to_string(static_cast<int>(hpMax)),
                      hpP, hpS);

    engine::TextPlacement livesP;
    livesP.pos = {8.0f, bottom - 34.0f};
    livesP.scale = 0.7f;
    engine::TextStyle livesS;
    livesS.fillColor = glm::vec4(1.0f);
    renderer.drawText(m_font, "Lives: " + std::to_string(m_lives), livesP,
                      livesS);

    engine::TextPlacement enemiesP;
    enemiesP.pos = {8.0f, bottom - 12.0f};
    enemiesP.scale = 0.7f;
    engine::TextStyle enemiesS;
    enemiesS.fillColor = glm::vec4(1.0f);
    renderer.drawText(m_font, "Enemies: " + std::to_string(enemies), enemiesP,
                      enemiesS);
  }
}

}  // namespace shooter
