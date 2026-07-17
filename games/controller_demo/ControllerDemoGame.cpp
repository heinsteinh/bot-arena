#include "games/controller_demo/ControllerDemoGame.hpp"

#include <cstdlib>
#include <glm/glm.hpp>
#include <string>

#include "engine/core/Input.hpp"
#include "engine/renderer/MatrixCamera.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ControllerComponents.hpp"

namespace controllerdemo {

namespace {
const char* kNames[4] = {"Fly", "Orbit", "Follow", "2D (top-down)"};
}

void ControllerDemoGame::onAttach() {
  m_screenshot = std::getenv("BOTARENA_SCREENSHOT") != nullptr;
  if (const char* c = std::getenv("BOTARENA_CTRL")) m_controller = std::atoi(c);

  m_camera = m_scene.createObject("Camera");
  m_camera.addComponent<engine::CameraComponent>();  // perspective, primary

  engine::SceneObject ground = m_scene.createObject("Ground");
  engine::TransformComponent& gt =
      ground.getComponent<engine::TransformComponent>();
  gt.translation = {0.0f, -0.15f, 0.0f};
  gt.scale = {24.0f, 0.3f, 24.0f};
  m_visuals.push_back(ground);

  const glm::vec3 spots[] = {
      {-4, 0.8f, -2}, {0, 0.8f, -4}, {4, 0.8f, -1}, {2, 0.8f, -7}};
  for (const glm::vec3& p : spots) {
    engine::SceneObject o = m_scene.createObject("Cube");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = p;
    t.scale = glm::vec3(0.9f);
    m_visuals.push_back(o);
  }
  m_followTarget = m_visuals[1];  // a cube the Follow camera tracks

  setController(m_controller);
}

void ControllerDemoGame::setController(int index) {
  m_controller = ((index % 4) + 4) % 4;
  if (m_camera.hasComponent<engine::FlyControllerComponent>())
    m_camera.removeComponent<engine::FlyControllerComponent>();
  if (m_camera.hasComponent<engine::OrbitControllerComponent>())
    m_camera.removeComponent<engine::OrbitControllerComponent>();
  if (m_camera.hasComponent<engine::FollowControllerComponent>())
    m_camera.removeComponent<engine::FollowControllerComponent>();
  if (m_camera.hasComponent<engine::Camera2DControllerComponent>())
    m_camera.removeComponent<engine::Camera2DControllerComponent>();

  // Roughly the centroid of the cube cluster; used to frame the Fly/Orbit
  // default poses on the scene instead of the controllers' generic defaults.
  const glm::vec3 sceneCenter{0.5f, 0.8f, -3.5f};

  engine::CameraComponent& cam =
      m_camera.getComponent<engine::CameraComponent>();
  cam.type = engine::ProjectionType::Perspective;
  if (m_controller == 0) {
    // FlyControllerComponent only steers via WASD; it never repositions the
    // entity itself, so give it a starting translation that, combined with
    // its default yaw/pitch look direction, actually faces the cubes.
    m_camera.getComponent<engine::TransformComponent>().translation = {
        -8.0f, 7.8f, -12.0f};
    m_camera.addComponent<engine::FlyControllerComponent>();
  } else if (m_controller == 1) {
    engine::OrbitControllerComponent& oc =
        m_camera.addComponent<engine::OrbitControllerComponent>();
    oc.targetPoint = sceneCenter;
    oc.yaw = 40.0f;
    oc.pitch = 25.0f;
    oc.distance = 16.0f;
  } else if (m_controller == 2) {
    engine::FollowControllerComponent& fc =
        m_camera.addComponent<engine::FollowControllerComponent>();
    fc.target = static_cast<entt::entity>(m_followTarget);
    fc.offset = {0.0f, 5.0f, 10.0f};
  } else {
    cam.type = engine::ProjectionType::Orthographic;
    cam.orthoSize = 24.0f;
    // Default ortho near/far (-1..1) assume a camera close to the action;
    // this controller's camera sits 25 units above the ground, so widen the
    // clip range to keep the ground and cubes from being culled.
    cam.orthoNear = 1.0f;
    cam.orthoFar = 60.0f;
    m_camera.getComponent<engine::TransformComponent>().translation = {0, 25,
                                                                       0};
    m_camera.addComponent<engine::Camera2DControllerComponent>();
  }
}

void ControllerDemoGame::onUpdate(float dt) {
  if (!m_screenshot && engine::Input::wasKeyPressed(engine::Key::Space))
    setController(m_controller + 1);
  m_scene.update(dt);
}

void ControllerDemoGame::ensureResources(engine::Renderer& renderer) {
  if (m_ready) return;
  const engine::ShaderHandle s = renderer.meshShader();
  m_groundMat = renderer.registry().registerMaterial(
      {{0.45f, 0.47f, 0.5f, 1.0f}, 0.0f, 0.9f, s});
  m_cubeMat = renderer.registry().registerMaterial(
      {{0.72f, 0.36f, 0.30f, 1.0f}, 0.0f, 0.6f, s});
  m_ready = true;
}

void ControllerDemoGame::onRender(engine::Renderer& renderer, int width,
                                  int height) {
  ensureResources(renderer);
  if (m_screenshot)
    m_scene.update(0.0f);  // apply the selected controller's pose
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
  const engine::CameraUniforms cu = m_scene.cameraUniforms(aspect);
  renderer.setCamera(cu);
  renderer.setLightDirection(glm::normalize(glm::vec3(0.5f, 0.7f, 0.35f)));
  renderer.setPointLights({});

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MatrixCamera meshCam(cu.view, cu.projection);
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(), meshCam);
  for (size_t i = 0; i < m_visuals.size(); ++i)
    meshes.submit(cube, i == 0 ? m_groundMat : m_cubeMat,
                  m_visuals[i]
                      .getComponent<engine::TransformComponent>()
                      .localTransform());

  if (m_font) {
    engine::TextStyle st;
    st.fillColor = {1.0f, 1.0f, 1.0f, 1.0f};
    st.outlineColor = {0.05f, 0.05f, 0.08f, 1.0f};
    st.outlineWidthPx = 2.0f;
    renderer.drawText(m_font,
                      std::string("Camera controller: ") +
                          kNames[m_controller] + " (Space cycles)",
                      engine::TextPlacement::screen({40.0f, 60.0f}, 0.6f), st);
  }
}

}  // namespace controllerdemo
