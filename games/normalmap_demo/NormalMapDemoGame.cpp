#include "games/normalmap_demo/NormalMapDemoGame.hpp"

#include <cmath>
#include <cstdlib>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "engine/assets/TextureLoader.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"

namespace normalmapdemo {

void NormalMapDemoGame::onAttach() {
  m_screenshot = std::getenv("BOTARENA_SCREENSHOT") != nullptr;
  if (const char* l = std::getenv("BOTARENA_LIGHT"))
    m_lightPreset = std::atoi(l);
  m_camera.setPerspective(55.0f, 16.0f / 9.0f, 0.1f, 100.0f);
  m_camera.lookAt({2.6f, 2.0f, 10.0f}, {0.0f, 1.6f, 0.0f});
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
  m_camera.setPerspective(55.0f, aspect, 0.1f, 100.0f);
  renderer.setCamera(m_camera);

  // Directional key grazes the walls -> drives both the PCF shadow map and the
  // parallax self-shadow. BOTARENA_LIGHT freezes two raking angles.
  float a = m_time * 0.5f;
  if (m_screenshot) a = m_lightPreset == 1 ? 2.3f : 0.7f;
  renderer.setLightDirection(
      glm::normalize(glm::vec3(std::cos(a), 0.30f, std::sin(a) * 0.4f + 0.7f)));
  std::vector<engine::PointLight> lights;
  engine::PointLight fill;
  fill.positionRadius = glm::vec4(0.0f, 2.0f, 6.0f, 24.0f);
  fill.color = glm::vec4(0.5f, 0.55f, 0.65f, 0.8f);  // dim, so the key reads
  lights.push_back(fill);
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(), m_camera);
  auto wall = [&](float x, engine::MaterialHandle mat) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), {x, 1.5f, 0.0f});
    m = glm::scale(m, {2.0f, 3.0f, 0.2f});
    meshes.submit(cube, mat, m);
  };
  wall(-4.4f, m_flatMat);
  wall(0.0f, m_mappedMat);
  wall(4.4f, m_parallaxMat);

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
