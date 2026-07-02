#include "games/models/ModelsGame.hpp"

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "engine/assets/MeshBounds.hpp"
#include "engine/core/AssetPath.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace models {

void ModelsGame::onAttach() {
  m_camera.setTarget({0.0f, 0.4f, 0.0f});
  m_camera.setOrbit(35.0f, 20.0f, 3.5f);

  const char* files[][2] = {
      {"Suzanne", "meshes/suzanne.obj"},     {"Teapot", "meshes/teapot.obj"},
      {"Sphere", "meshes/sphere.obj"},       {"Torus", "meshes/torus.obj"},
      {"Spaceship", "meshes/spaceship.obj"}, {"Statue", "meshes/statue.obj"},
  };
  for (const auto& f : files) {
    m_entries.push_back({f[0], f[1], engine::Model{}});
  }
}

void ModelsGame::onUpdate(float dt) {
  m_camera.update(dt);
  if (m_autoRotate) m_angle += dt * 0.6f;
}

void ModelsGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    m_groundMat = renderer.registry().registerMaterial(
        {{0.14f, 0.15f, 0.18f, 1.0f}, 0.0f, 0.9f, s});
    for (Entry& e : m_entries) {
      e.model =
          engine::loadModel(engine::assetPath(e.path), renderer.registry(), s);
    }
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_camera.resize(aspect);
  renderer.setCamera(m_camera.camera());

  std::vector<engine::PointLight> lights;
  engine::PointLight key;
  key.positionRadius = glm::vec4(2.0f, 3.0f, 2.0f, 15.0f);
  key.color = glm::vec4(1.0f, 0.97f, 0.9f, 2.5f);
  lights.push_back(key);
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(),
                              m_camera.camera());

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.55f, 0.0f});
  ground = glm::scale(ground, {8.0f, 0.05f, 8.0f});
  meshes.submit(cube, m_groundMat, ground);

  if (m_selected >= 0 && m_selected < static_cast<int>(m_entries.size()) &&
      m_entries[m_selected].model.valid) {
    const engine::Model& model = m_entries[m_selected].model;
    glm::mat4 m = glm::rotate(glm::mat4(1.0f), m_angle, {0.0f, 1.0f, 0.0f});
    m = m * engine::fitToUnitTransform(model.bounds);
    for (const engine::Submesh& sm : model.submeshes) {
      meshes.submit(sm.mesh, sm.material, m);
    }
  }
}

void ModelsGame::onImGuiRender() {
  ImGui::SetNextWindowPos(ImVec2(20, 250), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
  ImGui::Begin("Model Viewer");
  for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
    const bool ok = m_entries[i].model.valid;
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
