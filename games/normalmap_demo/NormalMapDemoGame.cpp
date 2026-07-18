#include "games/normalmap_demo/NormalMapDemoGame.hpp"

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "engine/assets/TextureLoader.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/SceneCamera.hpp"

namespace normalmapdemo {

void NormalMapDemoGame::onAttach() {
  m_screenshot = std::getenv("BOTARENA_SCREENSHOT") != nullptr;
  if (const char* l = std::getenv("BOTARENA_LIGHT"))
    m_lightPreset = std::atoi(l);

  engine::SceneObject cam = m_scene.createObject("Camera");
  cam.getComponent<engine::TransformComponent>() =
      engine::lookAtTransform({2.6f, 2.0f, 10.0f}, {0.0f, 1.6f, 0.0f});
  engine::CameraComponent cc;
  cc.fov = 55.0f;
  cc.perspNear = 0.1f;
  cc.perspFar = 100.0f;
  cc.primary = true;
  cam.addComponent<engine::CameraComponent>(cc);

  // Animated directional key (translation set each frame in onRender).
  m_keyLight = m_scene.createObject("KeyLight");
  m_keyLight.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Directional, glm::vec3(1.0f), 1.0f, 10.0f});

  // Dim point fill so the key reads.
  engine::SceneObject fill = m_scene.createObject("Fill");
  fill.getComponent<engine::TransformComponent>().translation = {0.0f, 2.0f,
                                                                 6.0f};
  fill.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Point, glm::vec3(0.5f, 0.55f, 0.65f), 0.8f, 24.0f});

  // Three walls (materials attached in ensureResources).
  const float xs[3] = {-4.4f, 0.0f, 4.4f};
  for (float x : xs) {
    engine::SceneObject o = m_scene.createObject("Wall");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = {x, 1.5f, 0.0f};
    t.scale = {2.0f, 3.0f, 0.2f};
    m_walls.push_back(o);
  }
}

void NormalMapDemoGame::onUpdate(float dt) { m_time += dt; }

void NormalMapDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  const std::string tex = std::string(BOTARENA_ASSET_DIR) + "/textures/";
  engine::Ref<engine::Texture2D> brickD =
      engine::loadTexture(tex + "brick_d.jpg");
  engine::Ref<engine::Texture2D> brickN =
      engine::loadTexture(tex + "brick_n.jpg");

  engine::Material flat;
  flat.baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
  flat.roughness = 0.7f;
  flat.shader = s;
  flat.albedo = brickD;
  m_flatMat = renderer.registry().registerMaterial(flat);

  engine::Material mapped = flat;
  mapped.normalMap = brickN;
  m_mappedMat = renderer.registry().registerMaterial(mapped);

  engine::Ref<engine::Texture2D> brickH =
      engine::loadTexture(tex + "brick_h.png");
  engine::Material parallax = mapped;
  parallax.heightMap = brickH;
  parallax.heightScale = 0.08f;
  m_parallaxMat = renderer.registry().registerMaterial(parallax);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  const engine::MaterialHandle wallMats[3] = {m_flatMat, m_mappedMat,
                                              m_parallaxMat};
  for (size_t i = 0; i < m_walls.size(); ++i) {
    m_walls[i].addComponent<engine::MeshComponent>(
        engine::MeshComponent{cube, wallMats[i]});
  }
  m_ready = true;
}

void NormalMapDemoGame::onRender(engine::Renderer& renderer, int width,
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
  // Directional key grazes the walls -> drives both the PCF shadow map and the
  // parallax self-shadow. BOTARENA_LIGHT freezes two raking angles.
  float a = m_time * 0.5f;
  if (m_screenshot) a = m_lightPreset == 1 ? 2.3f : 0.7f;
  m_keyLight.getComponent<engine::TransformComponent>().translation =
      glm::vec3(std::cos(a), 0.28f, std::sin(a) * 0.35f + 0.32f);

  m_scene.render(renderer, aspect);

  // Billboard labels over each wall.
  if (m_font) {
    engine::TextStyle st;
    st.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    st.outlineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    st.outlineWidthPx = 3.0f;
    renderer.drawText(
        m_font, "flat",
        engine::TextPlacement::cameraBillboard({-4.4f, 3.4f, 0.2f}, 0.006f),
        st);
    renderer.drawText(
        m_font, "normal",
        engine::TextPlacement::cameraBillboard({0.0f, 3.4f, 0.2f}, 0.006f), st);
    renderer.drawText(
        m_font, "parallax",
        engine::TextPlacement::cameraBillboard({4.4f, 3.4f, 0.2f}, 0.006f), st);
  }
}

}  // namespace normalmapdemo
