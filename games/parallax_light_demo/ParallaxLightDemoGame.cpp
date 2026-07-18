#include "games/parallax_light_demo/ParallaxLightDemoGame.hpp"

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <string>

#include "engine/assets/TextureLoader.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/SceneCamera.hpp"

namespace parallaxlightdemo {

void ParallaxLightDemoGame::onAttach() {
  m_screenshot = std::getenv("BOTARENA_SCREENSHOT") != nullptr;
  if (const char* l = std::getenv("BOTARENA_LIGHT"))
    m_lightPreset = std::atoi(l);

  engine::SceneObject cam = m_scene.createObject("Camera");
  cam.getComponent<engine::TransformComponent>() =
      engine::lookAtTransform({0.5f, 5.5f, 7.0f}, {0.0f, 0.0f, -0.5f});
  engine::CameraComponent cc;
  cc.fov = 55.0f;
  cc.perspNear = 0.1f;
  cc.perspFar = 100.0f;
  cc.primary = true;
  cam.addComponent<engine::CameraComponent>(cc);

  // Floor (material attached in ensureResources).
  m_floor = m_scene.createObject("Floor");
  {
    engine::TransformComponent& t =
        m_floor.getComponent<engine::TransformComponent>();
    t.translation = {0.0f, -0.15f, 0.0f};
    t.scale = {6.0f, 0.3f, 6.0f};
  }

  // Three orbiting point lights (positions set each frame in onRender);
  // color/intensity/radius are static.
  const glm::vec3 lightColors[3] = {
      {1.0f, 0.55f, 0.35f}, {0.35f, 0.7f, 1.0f}, {0.45f, 1.0f, 0.55f}};
  for (int i = 0; i < 3; ++i) {
    m_orbitLights[i] = m_scene.createObject("OrbitLight");
    m_orbitLights[i].addComponent<engine::LightComponent>(
        engine::LightComponent{engine::LightType::Point, lightColors[i], 4.5f,
                               12.0f});
  }

  // Static near-horizontal directional.
  engine::SceneObject sun = m_scene.createObject("Sun");
  sun.getComponent<engine::TransformComponent>().translation = {0.9f, 0.12f,
                                                                0.25f};
  sun.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Directional, glm::vec3(1.0f), 1.0f, 10.0f});
}

void ParallaxLightDemoGame::onUpdate(float dt) { m_time += dt; }

void ParallaxLightDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  const std::string tex = std::string(BOTARENA_ASSET_DIR) + "/textures/";
  engine::Material floor;
  floor.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  floor.roughness = 0.7f;
  floor.shader = s;
  floor.albedo = engine::loadTexture(tex + "brick_d.jpg");
  floor.normalMap = engine::loadTexture(tex + "brick_n.jpg");
  floor.heightMap = engine::loadTexture(tex + "brick_h.png");
  floor.heightScale = 0.04f;
  m_floorMat = renderer.registry().registerMaterial(floor);

  m_floor.addComponent<engine::MeshComponent>(
      engine::MeshComponent{renderer.unitCubeMesh(), m_floorMat});
  m_ready = true;
}

void ParallaxLightDemoGame::onRender(engine::Renderer& renderer, int width,
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

  // Three distinctly-colored point lights orbiting LOW, 120 apart: each casts
  // its own parallax self-shadow (gShadow.g/b/a), so a crevice shadowed from
  // one light but lit by another shows a colored self-shadow.
  float a = m_time * 0.7f;
  if (m_screenshot) {
    a = m_lightPreset == 2 ? 3.9f : (m_lightPreset == 1 ? 2.2f : 0.4f);
  }
  const float twoPi = 6.2831853f;
  for (int i = 0; i < 3; ++i) {
    const float ai = a + twoPi * static_cast<float>(i) / 3.0f;
    m_orbitLights[i].getComponent<engine::TransformComponent>().translation =
        glm::vec3(std::cos(ai) * 2.4f, 0.9f, std::sin(ai) * 2.4f);
  }

  m_scene.render(renderer, aspect);

  if (m_font) {
    engine::TextStyle st;
    st.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    st.outlineColor = {0.05f, 0.05f, 0.08f, 1.0f};
    st.outlineWidthPx = 2.0f;
    renderer.drawText(m_font, "Point-light parallax shadows",
                      engine::TextPlacement::screen({40.0f, 60.0f}, 0.7f), st);
  }
}

}  // namespace parallaxlightdemo
