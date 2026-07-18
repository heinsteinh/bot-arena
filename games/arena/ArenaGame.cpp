#include "games/arena/ArenaGame.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "engine/ai/Steering.hpp"
#include "engine/core/FixedTimestep.hpp"
#include "engine/core/Input.hpp"
#include "engine/gameplay/Combat.hpp"
#include "engine/physics/Collision.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ControllerComponents.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "games/arena/Components.hpp"

namespace arena {

void ArenaGame::onAttach() {
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
  oc.yaw = 45.0f;
  oc.pitch = 55.0f;  // elevation above target, same convention as before
  oc.distance = 18.0f;
  oc.maxDistance = 40.0f;

  spawnEntities();

  // Four static point lights, previously rebuilt every frame in onRender. Now
  // scene entities; Scene::render collects them via collectLights.
  {
    const glm::vec3 palette[4] = {{1.0f, 0.3f, 0.2f},
                                  {0.3f, 0.6f, 1.0f},
                                  {0.4f, 1.0f, 0.4f},
                                  {1.0f, 0.9f, 0.3f}};
    for (int i = 0; i < 4; ++i) {
      const float sx = (i & 1) ? 4.0f : -4.0f;
      const float sz = (i & 2) ? 4.0f : -4.0f;
      engine::SceneObject light = m_scene.createObject("PointLight");
      light.getComponent<engine::TransformComponent>().translation = {sx, 2.0f,
                                                                      sz};
      light.addComponent<engine::LightComponent>(engine::LightComponent{
          engine::LightType::Point, palette[i], 3.0f, 8.0f});
    }
  }

  // Deterministic warm-up so a single-frame (headless screenshot) capture shows
  // a genuinely simulated state — bots advanced along their velocities and some
  // already bounced off the walls. No input during warm-up, so the player
  // stays.
  for (int i = 0; i < 300; ++i) stepSim(1.0f / 60.0f);

  entt::registry& reg = m_scene.registry();
  for (const entt::entity e : reg.view<Health, Player>()) {
    spdlog::info("arena: after warmup HP={} kills={} deaths={}",
                 reg.get<Health>(e).current, m_kills, m_deaths);
  }
}

void ArenaGame::spawnEntities() {
  // Static walls + ground, reproducing the old translate*scale boxes exactly:
  // the unit cube spans [-1, 1], so a wall of dimension `size` needs a scale
  // of size * 0.5 (the ground submission never had that halving, so its
  // scale is the literal size).
  auto spawnWall = [this](const glm::vec3& center, const glm::vec3& size) {
    engine::SceneObject o = m_scene.createObject("Wall");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = center;
    t.scale = size * 0.5f;
    m_walls.push_back(o);
  };
  spawnWall({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f});
  spawnWall({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f});
  spawnWall({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});
  spawnWall({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});

  m_ground = m_scene.createObject("Ground");
  engine::TransformComponent& gt =
      m_ground.getComponent<engine::TransformComponent>();
  gt.translation = {0.0f, -0.05f, 0.0f};
  gt.scale = {20.0f, 0.05f, 20.0f};

  engine::SceneObject player = m_scene.createObject("Player");
  engine::TransformComponent& pt =
      player.getComponent<engine::TransformComponent>();
  pt.translation = {0.0f, 0.3f, 0.0f};
  pt.scale = glm::vec3(0.4f);
  player.addComponent<Velocity>(glm::vec3(0.0f));
  // Player/Bot are empty tag types; entt's emplace() returns void (not T&)
  // for those, so they can't go through SceneObject::addComponent<T>'s T&
  // signature -- emplace them on the registry directly.
  m_scene.registry().emplace<Player>(static_cast<entt::entity>(player));
  player.addComponent<Health>(Health{kPlayerMaxHealth, kPlayerMaxHealth});

  std::uniform_real_distribution<float> posD(-4.0f, 4.0f);
  std::uniform_real_distribution<float> velD(-1.0f, 1.0f);
  for (int i = 0; i < 48; ++i) {
    engine::SceneObject b = m_scene.createObject("Bot");
    engine::TransformComponent& bt =
        b.getComponent<engine::TransformComponent>();
    bt.translation = {posD(m_rng), 0.3f, posD(m_rng)};
    bt.scale = glm::vec3(0.3f);
    glm::vec3 v(velD(m_rng), 0.0f, velD(m_rng));
    v = glm::length(v) > 0.001f ? glm::normalize(v) * 2.0f
                                : glm::vec3(2.0f, 0.0f, 0.0f);
    b.addComponent<Velocity>(v);
    m_scene.registry().emplace<Bot>(static_cast<entt::entity>(b));
    b.addComponent<Health>(Health{kBotMaxHealth, kBotMaxHealth});
  }
}

void ArenaGame::onUpdate(float dt) {
  m_scene.update(dt);  // drives the camera's OrbitControllerComponent

  m_accumulator += dt;
  const float step = 1.0f / 60.0f;
  const engine::FixedStep fs = engine::fixedTimestep(m_accumulator, step, 5);
  m_accumulator = fs.remainder;
  for (int i = 0; i < fs.steps; ++i) stepSim(step);
}

void ArenaGame::stepSim(float dt) {
  entt::registry& reg = m_scene.registry();

  // Input -> player velocity (XZ plane).
  glm::vec3 dir(0.0f);
  if (engine::Input::isKeyDown(engine::Key::W)) dir.z -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::S)) dir.z += 1.0f;
  if (engine::Input::isKeyDown(engine::Key::A)) dir.x -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::D)) dir.x += 1.0f;
  auto players = reg.view<Velocity, Player>();
  for (const entt::entity e : players) {
    players.get<Velocity>(e).value = glm::length(dir) > 0.001f
                                         ? glm::normalize(dir) * 3.0f
                                         : glm::vec3(0.0f);
  }

  // Bots seek the player.
  glm::vec3 playerPos(0.0f);
  for (const entt::entity e : reg.view<engine::TransformComponent, Player>()) {
    playerPos = reg.get<engine::TransformComponent>(e).translation;
  }
  for (const entt::entity e :
       reg.view<engine::TransformComponent, Velocity, Bot>()) {
    engine::TransformComponent& tr = reg.get<engine::TransformComponent>(e);
    Velocity& v = reg.get<Velocity>(e);
    const Health& h = reg.get<Health>(e);
    const glm::vec3 force =
        engine::shouldFlee(h.current, h.max, kFleeFraction)
            ? engine::flee(tr.translation, v.value, playerPos, kBotMaxSpeed,
                           kBotMaxForce)
            : engine::seek(tr.translation, v.value, playerPos, kBotMaxSpeed,
                           kBotMaxForce);
    v.value += force * dt;
    v.value = engine::truncate(v.value, kBotMaxSpeed);
  }

  // Integrate.
  auto view = reg.view<engine::TransformComponent, Velocity>();
  for (const entt::entity e : view) {
    view.get<engine::TransformComponent>(e).translation +=
        view.get<Velocity>(e).value * dt;
  }

  // Agent-vs-agent collision (O(n^2)).
  std::vector<entt::entity> agents(view.begin(), view.end());
  for (std::size_t i = 0; i < agents.size(); ++i) {
    for (std::size_t j = i + 1; j < agents.size(); ++j) {
      engine::TransformComponent& ta =
          view.get<engine::TransformComponent>(agents[i]);
      Velocity& va = view.get<Velocity>(agents[i]);
      engine::TransformComponent& tb =
          view.get<engine::TransformComponent>(agents[j]);
      Velocity& vb = view.get<Velocity>(agents[j]);
      const engine::AgentPair r =
          engine::resolveAgentPair(ta.translation, va.value, ta.scale.x,
                                   tb.translation, vb.value, tb.scale.x);
      ta.translation = r.posA;
      va.value = r.velA;
      tb.translation = r.posB;
      vb.value = r.velB;
    }
  }

  // Wall bounce (unchanged) -- last, so separation never leaves an agent
  // outside the arena.
  const glm::vec3 boundsMin(-4.75f, -1.0f, -4.75f);
  const glm::vec3 boundsMax(4.75f, 10.0f, 4.75f);
  for (const entt::entity e : view) {
    engine::TransformComponent& tr = view.get<engine::TransformComponent>(e);
    Velocity& v = view.get<Velocity>(e);
    const engine::WallBounce wb = engine::resolveWallBounce(
        tr.translation, v.value, boundsMin, boundsMax, tr.scale.x);
    tr.translation = wb.position;
    v.value = wb.velocity;
  }

  // Combat: contact damage between the player and touching bots; bots regen
  // while not in contact.
  entt::entity playerEnt = entt::null;
  for (const entt::entity e : reg.view<engine::TransformComponent, Player>()) {
    playerEnt = e;
  }
  if (playerEnt != entt::null) {
    engine::TransformComponent& pt =
        reg.get<engine::TransformComponent>(playerEnt);
    Health& ph = reg.get<Health>(playerEnt);
    for (const entt::entity e :
         reg.view<engine::TransformComponent, Health, Bot>()) {
      engine::TransformComponent& bt = reg.get<engine::TransformComponent>(e);
      Health& bh = reg.get<Health>(e);
      const float dx = bt.translation.x - pt.translation.x;
      const float dz = bt.translation.z - pt.translation.z;
      const float dist = std::sqrt(dx * dx + dz * dz);
      if (dist < bt.scale.x + pt.scale.x + kContactMargin) {
        ph.current = engine::adjustHealth(ph.current, -kBotDps * dt, ph.max);
        bh.current = engine::adjustHealth(bh.current, -kPlayerDps * dt, bh.max);
      } else {
        bh.current = engine::adjustHealth(bh.current, kBotRegen * dt, bh.max);
      }
    }

    // Death / respawn.
    std::uniform_real_distribution<float> angleD(0.0f, 6.2831853f);
    for (const entt::entity e :
         reg.view<engine::TransformComponent, Health, Bot>()) {
      Health& bh = reg.get<Health>(e);
      if (bh.current <= 0.0f) {
        engine::TransformComponent& bt = reg.get<engine::TransformComponent>(e);
        const float a = angleD(m_rng);
        bt.translation = {std::cos(a) * kArenaEdge, 0.3f,
                          std::sin(a) * kArenaEdge};
        bh.current = bh.max;
        ++m_kills;
      }
    }
    if (ph.current <= 0.0f) {
      pt.translation = {0.0f, 0.3f, 0.0f};
      ph.current = ph.max;
      ++m_deaths;
    }
  }
}

void ArenaGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    m_wallMat = renderer.registry().registerMaterial(
        {{0.7f, 0.7f, 0.7f, 1.0f}, 0.0f, 0.5f, s});
    m_groundMat = renderer.registry().registerMaterial(
        {{0.3f, 0.3f, 0.33f, 1.0f}, 0.0f, 0.85f, s});
    m_playerMat = renderer.registry().registerMaterial(
        {{0.95f, 0.95f, 1.0f, 1.0f}, 0.1f, 0.25f, s});
    m_botMats[0] = renderer.registry().registerMaterial(
        {{0.9f, 0.3f, 0.2f, 1.0f}, 0.0f, 0.4f, s});
    m_botMats[1] = renderer.registry().registerMaterial(
        {{0.2f, 0.6f, 0.9f, 1.0f}, 0.0f, 0.4f, s});
    m_botMats[2] = renderer.registry().registerMaterial(
        {{0.4f, 0.85f, 0.3f, 1.0f}, 0.0f, 0.4f, s});
    m_botMats[3] = renderer.registry().registerMaterial(
        {{0.9f, 0.75f, 0.2f, 1.0f}, 1.0f, 0.3f, s});

    // Entities are created (transform-only) during onAttach's warm-up, before
    // the Renderer exists. Attach MeshComponent now, on the first onRender,
    // once the cube handle + materials are available.
    const engine::MeshHandle cube = renderer.unitCubeMesh();
    for (engine::SceneObject& wall : m_walls) {
      wall.addComponent<engine::MeshComponent>(
          engine::MeshComponent{cube, m_wallMat});
    }
    m_ground.addComponent<engine::MeshComponent>(
        engine::MeshComponent{cube, m_groundMat});

    entt::registry& reg = m_scene.registry();
    for (const entt::entity e : reg.view<Player>()) {
      reg.emplace<engine::MeshComponent>(e, cube, m_playerMat);
    }
    int botIdx = 0;
    for (const entt::entity e : reg.view<Bot>()) {
      reg.emplace<engine::MeshComponent>(e, cube, m_botMats[botIdx++ % 4]);
    }

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

  m_scene.render(renderer, aspect);

  if (m_font) {
    entt::registry& reg = m_scene.registry();
    float playerHp = 0.0f;
    float playerMax = 0.0f;
    for (const entt::entity e : reg.view<Health, Player>()) {
      playerHp = reg.get<Health>(e).current;
      playerMax = reg.get<Health>(e).max;
    }
    const float bottom = static_cast<float>(height);

    engine::TextPlacement hpP;
    hpP.pos = {8.0f, bottom - 34.0f};
    hpP.scale = 0.7f;
    engine::TextStyle hpS;
    hpS.fillColor = glm::vec4(1.0f);
    renderer.drawText(m_font,
                      "HP: " + std::to_string(static_cast<int>(playerHp)) +
                          " / " + std::to_string(static_cast<int>(playerMax)),
                      hpP, hpS);

    engine::TextPlacement killsP;
    killsP.pos = {8.0f, bottom - 12.0f};
    killsP.scale = 0.7f;
    engine::TextStyle killsS;
    killsS.fillColor = glm::vec4(1.0f);
    renderer.drawText(m_font, "Kills: " + std::to_string(m_kills), killsP,
                      killsS);
  }
}

}  // namespace arena
