#include "games/models/ModelsGame.hpp"

#include <imgui.h>

#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "engine/assets/ModelLoader.hpp"
#include "engine/core/AssetPath.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ControllerComponents.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/ModelComponent.hpp"

namespace models {

void ModelsGame::onAttach() {
  m_camera = m_scene.createObject("Camera");
  engine::CameraComponent& cam =
      m_camera.addComponent<engine::CameraComponent>();
  cam.fov =
      60.0f;  // matches old OrbitCameraController default (m_fovDegrees=60)
  cam.perspNear = 0.1f;
  cam.perspFar = 100.0f;
  engine::OrbitControllerComponent& oc =
      m_camera.addComponent<engine::OrbitControllerComponent>();
  oc.targetPoint = {0.0f, 0.4f, 0.0f};
  // Orbit yaw convention: OrbitControllerComponent's orbitPosition yields
  // pos = center + d*(cosP*sinY, sinP, cosP*cosY); the old setOrbit yielded
  // pos = center + d*(cosP*cosYold, sinP, cosP*sinYold). Matching them gives
  // Ynew = 90 - Yold (NOT Yold + 90 -- that only coincides at Yold=0). So the
  // old setOrbit(35, 20, 3.5) maps to yaw = 90 - 35 = 55.
  oc.yaw = 55.0f;
  oc.pitch = 20.0f;  // elevation above target
  oc.distance = 3.5f;
  oc.maxDistance = 40.0f;

  m_ground = m_scene.createObject("Ground");
  engine::TransformComponent& gt =
      m_ground.getComponent<engine::TransformComponent>();
  gt.translation = {0.0f, -0.55f, 0.0f};
  gt.scale = {8.0f, 0.05f, 8.0f};

  m_model = m_scene.createObject("Model");  // ModelComponent attached lazily

  const char* files[][2] = {
      {"Planet", "Objects/Planet/planet.obj"},
      {"Monitor", "meshes/monitor.obj"},
      {"Suzanne", "meshes/suzanne.obj"},
      {"Teapot", "meshes/teapot.obj"},
      {"Sphere", "meshes/sphere.obj"},
      {"Torus", "meshes/torus.obj"},
      {"Spaceship", "meshes/spaceship.obj"},
      {"Statue", "meshes/statue.obj"},
  };
  for (const auto& f : files) {
    m_entries.push_back({f[0], f[1], 0, false});
  }

  // Static point key light, previously rebuilt every frame in onRender.
  engine::SceneObject keyLight = m_scene.createObject("KeyLight");
  keyLight.getComponent<engine::TransformComponent>().translation = {2.0f, 3.0f,
                                                                     2.0f};
  keyLight.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Point, glm::vec3(1.0f, 0.97f, 0.9f), 2.5f, 15.0f});
}

void ModelsGame::onUpdate(float dt) {
  m_scene.update(dt);  // drives the camera's OrbitControllerComponent
  if (m_autoRotate) m_angle += dt * 0.6f;
  m_model.getComponent<engine::TransformComponent>().rotation =
      glm::angleAxis(m_angle, glm::vec3(0.0f, 1.0f, 0.0f));
}

void ModelsGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    engine::ResourceRegistry& reg = renderer.registry();
    m_groundMat =
        reg.registerMaterial({{0.14f, 0.15f, 0.18f, 1.0f}, 0.0f, 0.9f, s});
    m_ground.addComponent<engine::MeshComponent>(
        engine::MeshComponent{renderer.unitCubeMesh(), m_groundMat});
    for (Entry& e : m_entries) {
      const engine::Model m =
          engine::loadModel(engine::assetPath(e.path), reg, s);
      e.valid = m.valid;
      e.handle = reg.registerModel(m);
    }
    // Attach the model component pointing at the initially selected entry.
    m_model.addComponent<engine::ModelComponent>(
        engine::ModelComponent{m_entries[m_selected].handle});
    m_resourcesReady = true;
  }

  // Track the ImGui selection (mutate the handle in place, no re-create).
  if (m_selected >= 0 && m_selected < static_cast<int>(m_entries.size()) &&
      m_entries[m_selected].valid) {
    m_model.getComponent<engine::ModelComponent>().model =
        m_entries[m_selected].handle;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;

  m_scene.render(renderer, aspect);
}

void ModelsGame::onImGuiRender() {
  ImGui::SetNextWindowPos(ImVec2(20, 250), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
  ImGui::Begin("Model Viewer");
  for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
    const bool ok = m_entries[i].valid;
    if (ImGui::RadioButton(m_entries[i].name.c_str(), m_selected == i) && ok) {
      m_selected = i;
    }
    if (!ok) {
      ImGui::SameLine();
      ImGui::TextDisabled("(failed)");
    }
  }
  ImGui::Checkbox("Auto-rotate", &m_autoRotate);
  ImGui::End();
}

}  // namespace models
