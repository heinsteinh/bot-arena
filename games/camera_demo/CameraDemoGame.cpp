#include "games/camera_demo/CameraDemoGame.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "engine/core/Input.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"

namespace camerademo {

namespace {
constexpr int kViewCount = 4;
const char* kViewNames[kViewCount] = {"ORBIT", "FLY (FPS)", "TOP-DOWN (ortho)",
                                      "FRONT (fixed)"};

engine::TextPlacement at(float x, float y, float scale) {
  engine::TextPlacement p;
  p.pos = {x, y};
  p.scale = scale;
  return p;
}
}  // namespace

void CameraDemoGame::onAttach() {
  if (const char* v = std::getenv("BOTARENA_VIEW")) {
    const int idx = std::atoi(v);
    if (idx >= 0 && idx < kViewCount) m_view = idx;
  }
  m_fly.setPose({6.0f, 4.0f, 10.0f}, -120.0f, -18.0f);
}

void CameraDemoGame::onUpdate(float dt) {
  m_time += dt;
  if (engine::Input::wasKeyPressed(engine::Key::Space)) {
    m_view = (m_view + 1) % kViewCount;
  }
  // Orbit auto-rotates from m_time in activeCamera(); only fly reads input.
  if (m_view == 1) m_fly.update(dt);
}

const engine::Camera& CameraDemoGame::activeCamera(float aspect) {
  switch (m_view) {
    case 1:
      m_fly.resize(aspect);
      return m_fly.camera();
    case 2: {
      const float halfH = 12.0f;
      m_top.setBounds(-halfH * aspect, halfH * aspect, -halfH, halfH, -100.0f,
                      100.0f);
      m_top.lookAt({0.0f, 14.0f, 0.001f}, {0.0f, 0.0f, 0.0f});
      return m_top;
    }
    case 3:
      m_front.setPerspective(55.0f, aspect, 0.1f, 100.0f);
      m_front.lookAt({0.0f, 3.5f, 14.0f}, {0.0f, 1.0f, 0.0f});
      return m_front;
    case 0:
    default:
      m_orbit.setTarget({0.0f, 0.5f, 0.0f});
      m_orbit.setOrbit(40.0f + m_time * 18.0f, 28.0f, 16.0f);
      m_orbit.resize(aspect);
      return m_orbit.camera();
  }
}

void CameraDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_resourcesReady) return;
  const engine::ShaderHandle s = renderer.meshShader();
  m_groundMat = renderer.registry().registerMaterial(
      {{0.28f, 0.30f, 0.34f, 1.0f}, 0.0f, 0.9f, s});
  m_cubeMats[0] = renderer.registry().registerMaterial(
      {{0.90f, 0.30f, 0.25f, 1.0f}, 0.1f, 0.35f, s});
  m_cubeMats[1] = renderer.registry().registerMaterial(
      {{0.25f, 0.55f, 0.95f, 1.0f}, 0.1f, 0.35f, s});
  m_cubeMats[2] = renderer.registry().registerMaterial(
      {{0.35f, 0.85f, 0.40f, 1.0f}, 0.1f, 0.35f, s});
  m_cubeMats[3] = renderer.registry().registerMaterial(
      {{0.95f, 0.80f, 0.25f, 1.0f}, 0.6f, 0.30f, s});
  m_resourcesReady = true;
}

void CameraDemoGame::onRender(engine::Renderer& renderer, int width,
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
  const engine::Camera& cam = activeCamera(aspect);
  renderer.setCamera(cam);

  // Colored point lights so materials read under every view.
  std::vector<engine::PointLight> lights;
  const glm::vec3 palette[4] = {{1.0f, 0.4f, 0.3f},
                                {0.3f, 0.6f, 1.0f},
                                {0.4f, 1.0f, 0.5f},
                                {1.0f, 0.9f, 0.4f}};
  for (int i = 0; i < 4; ++i) {
    engine::PointLight pl;
    const float sx = (i & 1) ? 5.0f : -5.0f;
    const float sz = (i & 2) ? 5.0f : -5.0f;
    pl.positionRadius = glm::vec4(sx, 3.0f, sz, 12.0f);
    pl.color = glm::vec4(palette[i], 3.0f);
    lights.push_back(pl);
  }
  renderer.setPointLights(lights);

  // Scene: ground slab + a ring of cubes + a center pillar.
  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(), cam);

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.05f, 0.0f});
  ground = glm::scale(ground, {24.0f, 0.1f, 24.0f});
  meshes.submit(cube, m_groundMat, ground);

  const int ringCount = 8;
  for (int i = 0; i < ringCount; ++i) {
    const float a = glm::radians(360.0f / ringCount * static_cast<float>(i));
    const glm::vec3 pos{std::cos(a) * 5.0f, 0.75f, std::sin(a) * 5.0f};
    glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
    m = glm::rotate(m, a, {0.0f, 1.0f, 0.0f});
    m = glm::scale(m, glm::vec3(1.2f, 1.5f, 1.2f));
    meshes.submit(cube, m_cubeMats[i % 4], m);
  }
  glm::mat4 center = glm::translate(glm::mat4(1.0f), {0.0f, 1.0f, 0.0f});
  center = glm::scale(center, {1.5f, 2.0f, 1.5f});
  meshes.submit(cube, m_cubeMats[3], center);

  // HUD (SDF effects), clear of the top-left debug overlay.
  if (!m_font) return;
  const glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
  const glm::vec4 ink{0.05f, 0.05f, 0.08f, 1.0f};
  const float h = static_cast<float>(height);

  engine::TextStyle title;
  title.fillColor = white;
  title.outlineColor = ink;
  title.outlineWidthPx = 2.0f;
  renderer.drawText(m_font, "Camera Views", at(430.0f, 60.0f, 0.9f), title);

  engine::TextStyle name;
  name.fillColor = white;
  name.glowColor = {0.3f, 0.8f, 1.0f, 1.0f};
  name.glowSizePx = 9.0f;
  renderer.drawText(m_font, kViewNames[m_view], at(430.0f, 128.0f, 1.3f), name);

  // View list (active marked and brightened).
  for (int i = 0; i < kViewCount; ++i) {
    const bool active = i == m_view;
    engine::TextStyle s;
    s.fillColor = active ? glm::vec4{1.0f, 0.9f, 0.4f, 1.0f}
                         : glm::vec4{0.55f, 0.58f, 0.62f, 1.0f};
    const std::string line = std::string(active ? "> " : "  ") + kViewNames[i];
    renderer.drawText(m_font, line, at(40.0f, 300.0f + i * 34.0f, 0.6f), s);
  }

  // Camera position readout (generic: from the inverse view matrix).
  const glm::vec3 cp = glm::vec3(glm::inverse(cam.view())[3]);
  char buf[96];
  std::snprintf(buf, sizeof(buf), "eye  %.1f, %.1f, %.1f", cp.x, cp.y, cp.z);
  engine::TextStyle read;
  read.fillColor = {0.8f, 0.85f, 0.95f, 1.0f};
  renderer.drawText(m_font, buf, at(430.0f, 205.0f, 0.55f), read);

  // Controls hint with a drop shadow.
  engine::TextStyle hint;
  hint.fillColor = {0.8f, 0.8f, 0.85f, 1.0f};
  hint.shadowColor = {0.0f, 0.0f, 0.0f, 0.7f};
  hint.shadowOffsetPx = {2.0f, 2.0f};
  hint.shadowSoftnessPx = 2.0f;
  renderer.drawText(m_font, "Space: next view   |   WASD + right-drag: fly",
                    at(40.0f, h - 30.0f, 0.55f), hint);
}

}  // namespace camerademo
