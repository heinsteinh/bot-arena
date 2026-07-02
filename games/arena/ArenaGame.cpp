#include "games/arena/ArenaGame.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <string>
#include <vector>

#include "engine/ai/Steering.hpp"
#include "engine/core/FixedTimestep.hpp"
#include "engine/core/Input.hpp"
#include "engine/gameplay/Combat.hpp"
#include "engine/physics/Collision.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "games/arena/Components.hpp"

namespace arena {

void ArenaGame::onAttach() {
  m_camera.setTarget({0.0f, 0.5f, 0.0f});
  m_camera.setOrbit(45.0f, 55.0f, 18.0f);
  m_font = engine::Font::Load(
      std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf", 32);
  spawnEntities();
  // Deterministic warm-up so a single-frame (headless screenshot) capture shows
  // a genuinely simulated state — bots advanced along their velocities and some
  // already bounced off the walls. No input during warm-up, so the player
  // stays.
  for (int i = 0; i < 300; ++i) stepSim(1.0f / 60.0f);

  for (const entt::entity e : m_registry.view<Health, Player>()) {
    spdlog::info("arena: after warmup HP={} kills={} deaths={}",
                 m_registry.get<Health>(e).current, m_kills, m_deaths);
  }
}

void ArenaGame::spawnEntities() {
  const entt::entity player = m_registry.create();
  m_registry.emplace<Transform>(player, glm::vec3(0.0f, 0.3f, 0.0f), 0.4f);
  m_registry.emplace<Velocity>(player, glm::vec3(0.0f));
  m_registry.emplace<Player>(player);
  m_registry.emplace<Health>(player, kPlayerMaxHealth, kPlayerMaxHealth);

  std::uniform_real_distribution<float> posD(-4.0f, 4.0f);
  std::uniform_real_distribution<float> velD(-1.0f, 1.0f);
  for (int i = 0; i < 48; ++i) {
    const entt::entity b = m_registry.create();
    m_registry.emplace<Transform>(b, glm::vec3(posD(m_rng), 0.3f, posD(m_rng)),
                                  0.3f);
    glm::vec3 v(velD(m_rng), 0.0f, velD(m_rng));
    v = glm::length(v) > 0.001f ? glm::normalize(v) * 2.0f
                                : glm::vec3(2.0f, 0.0f, 0.0f);
    m_registry.emplace<Velocity>(b, v);
    m_registry.emplace<Bot>(b);
    m_registry.emplace<Health>(b, kBotMaxHealth, kBotMaxHealth);
  }
}

void ArenaGame::onUpdate(float dt) {
  m_camera.update(
      dt);  // apply setTarget/setOrbit (and mouse orbit) to the view

  m_accumulator += dt;
  const float step = 1.0f / 60.0f;
  const engine::FixedStep fs = engine::fixedTimestep(m_accumulator, step, 5);
  m_accumulator = fs.remainder;
  for (int i = 0; i < fs.steps; ++i) stepSim(step);
}

void ArenaGame::stepSim(float dt) {
  // Input -> player velocity (XZ plane).
  glm::vec3 dir(0.0f);
  if (engine::Input::isKeyDown(engine::Key::W)) dir.z -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::S)) dir.z += 1.0f;
  if (engine::Input::isKeyDown(engine::Key::A)) dir.x -= 1.0f;
  if (engine::Input::isKeyDown(engine::Key::D)) dir.x += 1.0f;
  auto players = m_registry.view<Velocity, Player>();
  for (const entt::entity e : players) {
    players.get<Velocity>(e).value = glm::length(dir) > 0.001f
                                         ? glm::normalize(dir) * 3.0f
                                         : glm::vec3(0.0f);
  }

  // Bots seek the player.
  glm::vec3 playerPos(0.0f);
  for (const entt::entity e : m_registry.view<Transform, Player>()) {
    playerPos = m_registry.get<Transform>(e).position;
  }
  for (const entt::entity e : m_registry.view<Transform, Velocity, Bot>()) {
    Transform& tr = m_registry.get<Transform>(e);
    Velocity& v = m_registry.get<Velocity>(e);
    const Health& h = m_registry.get<Health>(e);
    const glm::vec3 force = engine::shouldFlee(h.current, h.max, kFleeFraction)
                                ? engine::flee(tr.position, v.value, playerPos,
                                               kBotMaxSpeed, kBotMaxForce)
                                : engine::seek(tr.position, v.value, playerPos,
                                               kBotMaxSpeed, kBotMaxForce);
    v.value += force * dt;
    v.value = engine::truncate(v.value, kBotMaxSpeed);
  }

  // Integrate.
  auto view = m_registry.view<Transform, Velocity>();
  for (const entt::entity e : view) {
    view.get<Transform>(e).position += view.get<Velocity>(e).value * dt;
  }

  // Agent-vs-agent collision (O(n^2)).
  std::vector<entt::entity> agents(view.begin(), view.end());
  for (std::size_t i = 0; i < agents.size(); ++i) {
    for (std::size_t j = i + 1; j < agents.size(); ++j) {
      Transform& ta = view.get<Transform>(agents[i]);
      Velocity& va = view.get<Velocity>(agents[i]);
      Transform& tb = view.get<Transform>(agents[j]);
      Velocity& vb = view.get<Velocity>(agents[j]);
      const engine::AgentPair r = engine::resolveAgentPair(
          ta.position, va.value, ta.scale, tb.position, vb.value, tb.scale);
      ta.position = r.posA;
      va.value = r.velA;
      tb.position = r.posB;
      vb.value = r.velB;
    }
  }

  // Wall bounce (unchanged) -- last, so separation never leaves an agent
  // outside the arena.
  const glm::vec3 boundsMin(-4.75f, -1.0f, -4.75f);
  const glm::vec3 boundsMax(4.75f, 10.0f, 4.75f);
  for (const entt::entity e : view) {
    Transform& tr = view.get<Transform>(e);
    Velocity& v = view.get<Velocity>(e);
    const engine::WallBounce wb = engine::resolveWallBounce(
        tr.position, v.value, boundsMin, boundsMax, tr.scale);
    tr.position = wb.position;
    v.value = wb.velocity;
  }

  // Combat: contact damage between the player and touching bots; bots regen
  // while not in contact.
  entt::entity playerEnt = entt::null;
  for (const entt::entity e : m_registry.view<Transform, Player>()) {
    playerEnt = e;
  }
  if (playerEnt != entt::null) {
    Transform& pt = m_registry.get<Transform>(playerEnt);
    Health& ph = m_registry.get<Health>(playerEnt);
    for (const entt::entity e : m_registry.view<Transform, Health, Bot>()) {
      Transform& bt = m_registry.get<Transform>(e);
      Health& bh = m_registry.get<Health>(e);
      const float dx = bt.position.x - pt.position.x;
      const float dz = bt.position.z - pt.position.z;
      const float dist = std::sqrt(dx * dx + dz * dz);
      if (dist < bt.scale + pt.scale + kContactMargin) {
        ph.current = engine::adjustHealth(ph.current, -kBotDps * dt, ph.max);
        bh.current = engine::adjustHealth(bh.current, -kPlayerDps * dt, bh.max);
      } else {
        bh.current = engine::adjustHealth(bh.current, kBotRegen * dt, bh.max);
      }
    }

    // Death / respawn.
    std::uniform_real_distribution<float> angleD(0.0f, 6.2831853f);
    for (const entt::entity e : m_registry.view<Transform, Health, Bot>()) {
      Health& bh = m_registry.get<Health>(e);
      if (bh.current <= 0.0f) {
        Transform& bt = m_registry.get<Transform>(e);
        const float a = angleD(m_rng);
        bt.position = {std::cos(a) * kArenaEdge, 0.3f,
                       std::sin(a) * kArenaEdge};
        bh.current = bh.max;
        ++m_kills;
      }
    }
    if (ph.current <= 0.0f) {
      pt.position = {0.0f, 0.3f, 0.0f};
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
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_camera.resize(aspect);
  renderer.setCamera(m_camera.camera());

  std::vector<engine::PointLight> lights;
  const glm::vec3 palette[4] = {{1.0f, 0.3f, 0.2f},
                                {0.3f, 0.6f, 1.0f},
                                {0.4f, 1.0f, 0.4f},
                                {1.0f, 0.9f, 0.3f}};
  for (int i = 0; i < 4; ++i) {
    engine::PointLight pl;
    const float sx = (i & 1) ? 4.0f : -4.0f;
    const float sz = (i & 2) ? 4.0f : -4.0f;
    pl.positionRadius = glm::vec4(sx, 2.0f, sz, 8.0f);
    pl.color = glm::vec4(palette[i], 3.0f);
    lights.push_back(pl);
  }
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(),
                              m_camera.camera());
  auto wall = [&](const glm::vec3& center, const glm::vec3& size) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), center);
    t = glm::scale(t, size * 0.5f);
    meshes.submit(cube, m_wallMat, t);
  };
  wall({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f});
  wall({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f});
  wall({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});
  wall({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f});

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.05f, 0.0f});
  ground = glm::scale(ground, {20.0f, 0.05f, 20.0f});
  meshes.submit(cube, m_groundMat, ground);

  int idx = 0;
  auto view = m_registry.view<Transform>();
  for (const entt::entity e : view) {
    const Transform& tr = view.get<Transform>(e);
    const engine::MaterialHandle mat =
        m_registry.all_of<Player>(e) ? m_playerMat : m_botMats[(idx++) % 4];
    glm::mat4 m = glm::translate(glm::mat4(1.0f), tr.position);
    m = glm::scale(m, glm::vec3(tr.scale));
    meshes.submit(cube, mat, m);
  }

  if (m_font) {
    float playerHp = 0.0f;
    float playerMax = 0.0f;
    for (const entt::entity e : m_registry.view<Health, Player>()) {
      playerHp = m_registry.get<Health>(e).current;
      playerMax = m_registry.get<Health>(e).max;
    }
    const float bottom = static_cast<float>(height);
    renderer.drawText(*m_font,
                      "HP: " + std::to_string(static_cast<int>(playerHp)) +
                          " / " + std::to_string(static_cast<int>(playerMax)),
                      8.0f, bottom - 34.0f, 0.7f, glm::vec4(1.0f));
    renderer.drawText(*m_font, "Kills: " + std::to_string(m_kills), 8.0f,
                      bottom - 12.0f, 0.7f, glm::vec4(1.0f));
  }
}

}  // namespace arena
