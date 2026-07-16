#include "games/parallax_light_demo/ParallaxLightDemoGame.hpp"

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

namespace parallaxlightdemo {

void ParallaxLightDemoGame::onAttach() {
  m_screenshot = std::getenv("BOTARENA_SCREENSHOT") != nullptr;
  if (const char* l = std::getenv("BOTARENA_LIGHT"))
    m_lightPreset = std::atoi(l);
  m_camera.setPerspective(55.0f, 16.0f / 9.0f, 0.1f, 100.0f);
  m_camera.lookAt({0.5f, 5.5f, 7.0f}, {0.0f, 0.0f, -0.5f});
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
  m_camera.setPerspective(55.0f, aspect, 0.1f, 100.0f);
  renderer.setCamera(m_camera);

  // Three distinctly-colored point lights orbiting LOW, 120 apart: each casts
  // its own parallax self-shadow (gShadow.g/b/a), so a crevice shadowed from
  // one light but lit by another shows a colored self-shadow.
  float a = m_time * 0.7f;
  if (m_screenshot) {
    a = m_lightPreset == 2 ? 3.9f : (m_lightPreset == 1 ? 2.2f : 0.4f);
  }
  const glm::vec3 lightColors[3] = {{1.0f, 0.55f, 0.35f},   // warm orange
                                    {0.35f, 0.7f, 1.0f},    // cool blue
                                    {0.45f, 1.0f, 0.55f}};  // green
  const float twoPi = 6.2831853f;
  std::vector<engine::PointLight> lights;
  for (int i = 0; i < 3; ++i) {
    const float ai = a + twoPi * static_cast<float>(i) / 3.0f;
    engine::PointLight pl;
    pl.positionRadius =
        glm::vec4(std::cos(ai) * 2.4f, 0.9f, std::sin(ai) * 2.4f, 12.0f);
    pl.color = glm::vec4(lightColors[i], 4.5f);
    lights.push_back(pl);
  }
  renderer.setPointLights(lights);
  // Directional nearly straight down -> flat fill, so the point light is the
  // unmistakable self-shadow caster.
  renderer.setLightDirection(glm::normalize(glm::vec3(0.1f, 1.0f, 0.15f)));

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(), m_camera);
  glm::mat4 floorM = glm::translate(glm::mat4(1.0f), {0.0f, -0.15f, 0.0f});
  floorM = glm::scale(floorM, {6.0f, 0.3f, 6.0f});
  meshes.submit(cube, m_floorMat, floorM);

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
