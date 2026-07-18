#include "games/csm_demo/CsmDemoGame.hpp"

#include <glm/glm.hpp>
#include <string>

#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/SceneCamera.hpp"

namespace csmdemo {

void CsmDemoGame::onAttach() {
  engine::SceneObject cam = m_scene.createObject("Camera");
  cam.getComponent<engine::TransformComponent>() =
      engine::lookAtTransform({5.0f, 7.5f, 9.0f}, {-1.0f, 0.0f, -13.0f});
  engine::CameraComponent cc;
  cc.fov = 55.0f;
  cc.perspNear = 0.1f;
  cc.perspFar = 100.0f;
  cc.primary = true;
  cam.addComponent<engine::CameraComponent>(cc);

  // Low sun from the side -> long pillar shadows (translation is the toward-
  // light vector; Scene::render normalizes it).
  engine::SceneObject sun = m_scene.createObject("Sun");
  sun.getComponent<engine::TransformComponent>().translation = {0.62f, 0.5f,
                                                                0.12f};
  sun.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Directional, glm::vec3(1.0f), 1.0f, 10.0f});

  // Ground plane running away along -Z.
  engine::SceneObject ground = m_scene.createObject("Ground");
  {
    engine::TransformComponent& t =
        ground.getComponent<engine::TransformComponent>();
    t.translation = {0.0f, -0.1f, -18.0f};
    t.scale = {30.0f, 0.2f, 60.0f};
  }
  m_visuals.push_back(ground);

  // Row of pillars receding into the distance.
  for (int i = 0; i < 8; ++i) {
    engine::SceneObject o = m_scene.createObject("Pillar");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = {0.0f, 1.4f, -2.0f - static_cast<float>(i) * 5.0f};
    t.scale = {0.8f, 2.8f, 0.8f};
    m_visuals.push_back(o);
  }
}

void CsmDemoGame::onUpdate(float dt) { m_time += dt; }

void CsmDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  m_groundMat = renderer.registry().registerMaterial(
      {{0.55f, 0.55f, 0.58f, 1.0f}, 0.0f, 0.9f, s});
  m_pillarMat = renderer.registry().registerMaterial(
      {{0.70f, 0.35f, 0.30f, 1.0f}, 0.0f, 0.6f, s});

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  for (size_t i = 0; i < m_visuals.size(); ++i) {
    const engine::MaterialHandle mat = i == 0 ? m_groundMat : m_pillarMat;
    m_visuals[i].addComponent<engine::MeshComponent>(
        engine::MeshComponent{cube, mat});
  }
  m_ready = true;
}

void CsmDemoGame::onRender(engine::Renderer& renderer, int width, int height) {
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

  if (m_font) {
    engine::TextStyle st;
    st.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    st.outlineColor = {0.05f, 0.05f, 0.08f, 1.0f};
    st.outlineWidthPx = 2.0f;
    renderer.drawText(m_font, "Cascaded shadow maps (BOTARENA_CSM=1 to tint)",
                      engine::TextPlacement::screen({40.0f, 60.0f}, 0.6f), st);
  }
}

}  // namespace csmdemo
