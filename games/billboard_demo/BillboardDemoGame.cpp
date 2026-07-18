#include "games/billboard_demo/BillboardDemoGame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/SceneCamera.hpp"

namespace billboarddemo {

namespace {
constexpr float kLifetime = 1.2f;
constexpr float kRise = 1.2f;
constexpr float kWorldUnitsPerPixel = 0.012f;

// Color by magnitude: small white, big orange, crit yellow.
glm::vec4 damageColor(int value, bool crit) {
  if (crit) return {1.0f, 0.9f, 0.2f, 1.0f};
  if (value >= 50) return {1.0f, 0.5f, 0.2f, 1.0f};
  return {1.0f, 1.0f, 1.0f, 1.0f};
}
}  // namespace

void BillboardDemoGame::onAttach() {
  m_screenshot = std::getenv("BOTARENA_SCREENSHOT") != nullptr;

  // Camera: non-zero yaw AND pitch; BOTARENA_ORBIT picks a preset for the
  // rotated-camera validation.
  float yaw = 35.0f;
  float pitch = 22.0f;
  if (const char* o = std::getenv("BOTARENA_ORBIT")) {
    const int p = std::atoi(o);
    if (p == 1) {
      yaw = 125.0f;
      pitch = 30.0f;
    }
    if (p == 2) {
      yaw = -70.0f;
      pitch = 15.0f;
    }
  }
  const glm::vec3 target{0.0f, 0.5f, 0.0f};
  const float yawR = glm::radians(yaw);
  const float pitchR = glm::radians(pitch);
  const glm::vec3 offset{std::cos(pitchR) * std::cos(yawR), std::sin(pitchR),
                         std::cos(pitchR) * std::sin(yawR)};
  engine::SceneObject cam = m_scene.createObject("Camera");
  cam.getComponent<engine::TransformComponent>() =
      engine::lookAtTransform(target + offset * 15.0f, target);
  engine::CameraComponent cc;
  cc.fov = 60.0f;
  cc.perspNear = 0.1f;
  cc.perspFar = 100.0f;
  cc.primary = true;
  cam.addComponent<engine::CameraComponent>(cc);

  engine::SceneObject key = m_scene.createObject("KeyLight");
  key.getComponent<engine::TransformComponent>().translation = {3.0f, 6.0f,
                                                                3.0f};
  key.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Point, glm::vec3(1.0f, 0.95f, 0.9f), 3.0f, 20.0f});

  // Enemies in a ring.
  const int n = 6;
  for (int i = 0; i < n; ++i) {
    const float a = glm::radians(360.0f / n * static_cast<float>(i));
    m_enemies.push_back({std::cos(a) * 5.0f, 0.6f, std::sin(a) * 5.0f});
  }

  engine::SceneObject ground = m_scene.createObject("Ground");
  {
    engine::TransformComponent& t =
        ground.getComponent<engine::TransformComponent>();
    t.translation = {0.0f, -0.05f, 0.0f};
    t.scale = {24.0f, 0.1f, 24.0f};
  }
  m_visuals.push_back(ground);
  for (const glm::vec3& e : m_enemies) {
    engine::SceneObject o = m_scene.createObject("Enemy");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = e;
    t.scale = {1.0f, 1.2f, 1.0f};
    m_visuals.push_back(o);
  }

  // Pre-seed damage numbers over several enemies at different distances, with a
  // crit and a partially-faded one, not overlapping.
  const int seed[] = {12, 34, 88, 7, 56};
  const bool crit[] = {false, false, true, false, false};
  const float ages[] = {0.1f, 0.3f, 0.05f, 0.8f, 0.5f};
  for (int i = 0; i < 5; ++i) {
    DamageNumber d;
    d.worldPos = m_enemies[i % m_enemies.size()] + glm::vec3(0.0f, 1.1f, 0.0f);
    d.text = std::to_string(seed[i]);
    d.color = damageColor(seed[i], crit[i]);
    d.scaleMul = crit[i] ? 1.6f : 1.0f;
    d.age = ages[i];
    m_numbers.push_back(d);
  }
}

void BillboardDemoGame::onUpdate(float dt) {
  if (m_screenshot) return;  // deterministic frame: no motion/spawns
  for (DamageNumber& d : m_numbers) {
    d.age += dt;
    d.worldPos.y += kRise * dt;
  }
  m_numbers.erase(
      std::remove_if(m_numbers.begin(), m_numbers.end(),
                     [](const DamageNumber& d) { return d.age >= kLifetime; }),
      m_numbers.end());

  // Continuously spawn new hits over random enemies so numbers keep flowing.
  m_spawnTimer -= dt;
  while (m_spawnTimer <= 0.0f && !m_enemies.empty()) {
    m_spawnTimer += 0.4f;
    std::uniform_int_distribution<int> pick(
        0, static_cast<int>(m_enemies.size()) - 1);
    std::uniform_int_distribution<int> roll(1, 120);
    const int value = roll(m_rng);
    const bool crit = value >= 100;
    DamageNumber d;
    d.worldPos = m_enemies[pick(m_rng)] + glm::vec3(0.0f, 1.1f, 0.0f);
    d.text = std::to_string(value);
    d.color = damageColor(value, crit);
    d.scaleMul = crit ? 1.6f : 1.0f;
    m_numbers.push_back(d);
  }
}

void BillboardDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  m_groundMat = renderer.registry().registerMaterial(
      {{0.30f, 0.32f, 0.36f, 1.0f}, 0.0f, 0.9f, s});
  m_enemyMat = renderer.registry().registerMaterial(
      {{0.75f, 0.25f, 0.22f, 1.0f}, 0.1f, 0.4f, s});

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  for (size_t i = 0; i < m_visuals.size(); ++i) {
    const engine::MaterialHandle mat = i == 0 ? m_groundMat : m_enemyMat;
    m_visuals[i].addComponent<engine::MeshComponent>(
        engine::MeshComponent{cube, mat});
  }
  m_ready = true;
}

void BillboardDemoGame::onRender(engine::Renderer& renderer, int width,
                                 int height) {
  ensureResources(renderer);
  if (!m_font) {
    engine::FontDesc desc;
    desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf";
    desc.pixelSize = 48;
    desc.backend = engine::FontBackend::SDF;
    m_font = renderer.fonts().load(desc);
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_scene.render(renderer, aspect);

  // Damage numbers as camera-facing billboards (SDF, outlined, faded).
  if (m_font) {
    for (const DamageNumber& d : m_numbers) {
      const float fade = 1.0f - d.age / kLifetime;
      engine::TextStyle st;
      st.fillColor = {d.color.r, d.color.g, d.color.b, d.color.a * fade};
      st.outlineColor = {0.0f, 0.0f, 0.0f, fade};
      st.outlineWidthPx = 3.0f;
      renderer.drawText(m_font, d.text,
                        engine::TextPlacement::cameraBillboard(
                            d.worldPos, kWorldUnitsPerPixel * d.scaleMul),
                        st);
    }
  }

  // Screen HUD (proves overlay ordering: HUD over billboards).
  if (m_font) {
    engine::TextStyle title;
    title.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    title.outlineColor = {0.05f, 0.05f, 0.08f, 1.0f};
    title.outlineWidthPx = 2.0f;
    renderer.drawText(m_font, "Billboard damage numbers",
                      engine::TextPlacement::screen({430.0f, 60.0f}, 0.8f),
                      title);
  }
}

}  // namespace billboarddemo
