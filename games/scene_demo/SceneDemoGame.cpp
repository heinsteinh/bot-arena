#include "games/scene_demo/SceneDemoGame.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

#include "engine/renderer/MatrixCamera.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"

namespace scenedemo {

void SceneDemoGame::onAttach() {
  // Primary camera: above/behind the origin, tilted down toward -Z.
  m_camera = m_scene.createObject("Camera");
  engine::TransformComponent& ct =
      m_camera.getComponent<engine::TransformComponent>();
  ct.translation = {0.0f, 4.0f, 9.0f};
  ct.rotation = {glm::radians(-24.0f), 0.0f, 0.0f};
  engine::CameraComponent cam;
  cam.type = engine::ProjectionType::Perspective;
  cam.fov = 55.0f;
  cam.primary = true;
  m_camera.addComponent<engine::CameraComponent>(cam);

  // Ground.
  engine::SceneObject ground = m_scene.createObject("Ground");
  engine::TransformComponent& gt =
      ground.getComponent<engine::TransformComponent>();
  gt.translation = {0.0f, -0.15f, 0.0f};
  gt.scale = {16.0f, 0.3f, 16.0f};
  m_visuals.push_back(ground);

  // A small arrangement of cubes at distinct transforms.
  const glm::vec3 spots[] = {{-3.0f, 0.8f, -1.0f},
                             {0.0f, 0.8f, -3.0f},
                             {3.0f, 0.8f, 0.0f},
                             {1.5f, 0.8f, -5.0f}};
  for (const glm::vec3& p : spots) {
    engine::SceneObject o = m_scene.createObject("Cube");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = p;
    t.scale = glm::vec3(0.9f);
    m_visuals.push_back(o);
  }
}

void SceneDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  m_groundMat = renderer.registry().registerMaterial(
      {{0.45f, 0.47f, 0.5f, 1.0f}, 0.0f, 0.9f, s});
  m_cubeMat = renderer.registry().registerMaterial(
      {{0.72f, 0.36f, 0.30f, 1.0f}, 0.0f, 0.6f, s});
  m_ready = true;
}

void SceneDemoGame::onRender(engine::Renderer& renderer, int width,
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

  // The renderer's camera comes entirely from the scene's primary camera.
  const engine::CameraUniforms cu = m_scene.cameraUniforms(aspect);
  renderer.setCamera(cu);
  renderer.setLightDirection(glm::normalize(glm::vec3(0.5f, 0.7f, 0.35f)));
  renderer.setPointLights({});

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MatrixCamera meshCam(cu.view, cu.projection);
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(), meshCam);
  for (size_t i = 0; i < m_visuals.size(); ++i) {
    const engine::MaterialHandle mat = i == 0 ? m_groundMat : m_cubeMat;
    meshes.submit(cube, mat,
                  m_visuals[i]
                      .getComponent<engine::TransformComponent>()
                      .localTransform());
  }

  if (m_font) {
    engine::TextStyle st;
    st.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    st.outlineColor = {0.05f, 0.05f, 0.08f, 1.0f};
    st.outlineWidthPx = 2.0f;
    renderer.drawText(m_font, "Scene + SceneObject (primary camera from scene)",
                      engine::TextPlacement::screen({40.0f, 60.0f}, 0.6f), st);
  }
}

}  // namespace scenedemo
