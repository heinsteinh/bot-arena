#include "games/csm_demo/CsmDemoGame.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"

namespace csmdemo {

void CsmDemoGame::onAttach() {
  m_camera.setPerspective(55.0f, 16.0f / 9.0f, 0.1f, 100.0f);
  m_camera.lookAt({5.0f, 7.5f, 9.0f}, {-1.0f, 0.0f, -13.0f});
}

void CsmDemoGame::onUpdate(float dt) { m_time += dt; }

void CsmDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  m_groundMat = renderer.registry().registerMaterial(
      {{0.55f, 0.55f, 0.58f, 1.0f}, 0.0f, 0.9f, s});
  m_pillarMat = renderer.registry().registerMaterial(
      {{0.70f, 0.35f, 0.30f, 1.0f}, 0.0f, 0.6f, s});
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
  m_camera.setPerspective(55.0f, aspect, 0.1f, 100.0f);
  renderer.setCamera(m_camera);

  // Low sun from the side -> long pillar shadows cast across the ground.
  renderer.setLightDirection(glm::normalize(glm::vec3(0.62f, 0.5f, 0.12f)));
  renderer.setPointLights({});

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(), m_camera);

  // Long ground plane running away from the camera along -Z.
  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.1f, -18.0f});
  ground = glm::scale(ground, {30.0f, 0.2f, 60.0f});
  meshes.submit(cube, m_groundMat, ground);

  // A row of pillars receding into the distance -> spans all 3 cascades.
  for (int i = 0; i < 8; ++i) {
    const float z = -2.0f - static_cast<float>(i) * 5.0f;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), {0.0f, 1.4f, z});
    m = glm::scale(m, {0.8f, 2.8f, 0.8f});
    meshes.submit(cube, m_pillarMat, m);
  }

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
