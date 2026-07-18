#include "games/particles/ParticlesGame.hpp"

#include <imgui.h>

#include <cmath>
#include <vector>

#include "engine/particles/Particle.hpp"
#include "engine/renderer/ParticleInstance.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightComponent.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/SceneCamera.hpp"

namespace particles {

namespace {
engine::EmitParams burstParams() {
  engine::EmitParams p;
  p.count = 40;
  p.speedMin = 2.0f;
  p.speedMax = 5.0f;
  p.direction = {0.0f, 0.0f, 0.0f};  // radial
  p.spread = 1.0f;
  p.color = {1.0f, 0.5f, 0.15f};  // warm
  p.sizeMin = 0.05f;
  p.sizeMax = 0.12f;
  p.lifeMin = 0.4f;
  p.lifeMax = 0.9f;
  p.gravity = {0.0f, -6.0f, 0.0f};
  return p;
}
engine::EmitParams fountainParams() {
  engine::EmitParams p;
  p.count = 6;
  p.speedMin = 4.0f;
  p.speedMax = 6.0f;
  p.direction = {0.0f, 1.0f, 0.0f};
  p.spread = 0.35f;
  p.color = {0.2f, 0.7f, 1.0f};  // bright cyan
  p.sizeMin = 0.05f;
  p.sizeMax = 0.09f;
  p.lifeMin = 0.8f;
  p.lifeMax = 1.4f;
  p.gravity = {0.0f, -7.0f, 0.0f};
  return p;
}
engine::EmitParams smokeParams() {
  engine::EmitParams p;
  p.count = 3;
  p.speedMin = 0.4f;
  p.speedMax = 0.9f;
  p.direction = {0.0f, 1.0f, 0.0f};
  p.spread = 0.15f;
  p.count = 2;
  p.color = {0.1f, 0.12f, 0.16f};  // dim cool haze (additive stacks up)
  p.sizeMin = 0.18f;
  p.sizeMax = 0.3f;
  p.lifeMin = 1.8f;
  p.lifeMax = 2.8f;
  p.gravity = {0.0f, 0.3f, 0.0f};  // slight rise
  return p;
}
engine::EmitParams jetParams() {
  engine::EmitParams p;
  p.count = 4;
  p.speedMin = 8.0f;
  p.speedMax = 11.0f;
  p.direction = {0.0f, 1.0f, 0.0f};
  p.spread = 0.05f;
  p.color = {0.6f, 0.9f, 1.0f};
  p.sizeMin = 0.04f;
  p.sizeMax = 0.07f;
  p.lifeMin = 0.6f;
  p.lifeMax = 1.0f;
  p.gravity = {0.0f, -5.0f, 0.0f};
  return p;
}
engine::EmitParams novaParams() {
  engine::EmitParams p;
  p.count = 60;
  p.speedMin = 6.0f;
  p.speedMax = 10.0f;
  p.direction = {0.0f, 0.0f, 0.0f};  // radial
  p.spread = 1.0f;
  p.color = {1.0f, 0.8f, 0.3f};  // gold
  p.sizeMin = 0.08f;
  p.sizeMax = 0.15f;
  p.lifeMin = 0.7f;
  p.lifeMax = 1.2f;
  p.gravity = {0.0f, -3.0f, 0.0f};
  return p;
}
engine::EmitParams drizzleParams() {
  engine::EmitParams p;
  p.count = 5;
  p.speedMin = 3.0f;
  p.speedMax = 5.0f;
  p.direction = {0.0f, 1.0f, 0.0f};
  p.spread = 0.4f;
  p.color = {0.4f, 0.85f, 0.6f};
  p.sizeMin = 0.05f;
  p.sizeMax = 0.09f;
  p.lifeMin = 1.2f;
  p.lifeMax = 2.0f;
  p.gravity = {0.0f, -9.0f, 0.0f};
  return p;
}

struct NamedPreset {
  const char* name;
  engine::EmitParams (*make)();
};
const NamedPreset kPresets[] = {
    {"Burst", burstParams}, {"Fountain", fountainParams},
    {"Smoke", smokeParams}, {"Jet", jetParams},
    {"Nova", novaParams},   {"Drizzle", drizzleParams},
};
constexpr int kPresetCount = 6;
}  // namespace

void ParticlesGame::onAttach() {
  const glm::vec3 target{0.0f, 1.0f, 0.0f};
  const float yawR = glm::radians(40.0f);
  const float pitchR = glm::radians(25.0f);
  const glm::vec3 offset{std::cos(pitchR) * std::cos(yawR), std::sin(pitchR),
                         std::cos(pitchR) * std::sin(yawR)};
  engine::SceneObject cam = m_scene.createObject("Camera");
  cam.getComponent<engine::TransformComponent>() =
      engine::lookAtTransform(target + offset * 14.0f, target);
  engine::CameraComponent cc;
  cc.fov = 60.0f;
  cc.perspNear = 0.1f;
  cc.perspFar = 100.0f;
  cc.primary = true;
  cam.addComponent<engine::CameraComponent>(cc);

  engine::SceneObject key = m_scene.createObject("KeyLight");
  key.getComponent<engine::TransformComponent>().translation = {0.0f, 6.0f,
                                                                0.0f};
  key.addComponent<engine::LightComponent>(engine::LightComponent{
      engine::LightType::Point, glm::vec3(1.0f, 1.0f, 1.0f), 2.0f, 20.0f});

  m_ground = m_scene.createObject("Ground");
  {
    engine::TransformComponent& t =
        m_ground.getComponent<engine::TransformComponent>();
    t.translation = {0.0f, -0.05f, 0.0f};
    t.scale = {20.0f, 0.05f, 20.0f};
  }

  std::uniform_real_distribution<float> posD(-2.0f, 2.0f);
  std::uniform_real_distribution<float> velD(-3.5f, 3.5f);
  for (int i = 0; i < 6; ++i) {
    m_bouncers.push_back(
        {{posD(m_rng), 1.0f, posD(m_rng)}, {velD(m_rng), 0.0f, velD(m_rng)}});
  }

  for (size_t i = 0; i < m_bouncers.size(); ++i) {
    engine::SceneObject o = m_scene.createObject("Bouncer");
    engine::TransformComponent& t =
        o.getComponent<engine::TransformComponent>();
    t.translation = m_bouncers[i].position;
    t.scale = glm::vec3(0.3f);
    m_bouncerObjs.push_back(o);
  }

  m_editorParams = kPresets[m_selectedPreset].make();

  for (int i = 0; i < 90; ++i) stepSim(1.0f / 60.0f);
}

void ParticlesGame::onUpdate(float dt) {
  m_accumulator += dt;
  const float step = 1.0f / 60.0f;
  int steps = 0;
  while (m_accumulator >= step && steps < 5) {
    stepSim(step);
    m_accumulator -= step;
    ++steps;
  }
}

void ParticlesGame::stepSim(float dt) {
  for (Bouncer& b : m_bouncers) {
    b.position += b.velocity * dt;
    for (int a = 0; a < 3; a += 2) {  // x and z walls
      bool bounced = false;
      if (b.position[a] > 3.0f) {
        b.position[a] = 3.0f;
        b.velocity[a] = -b.velocity[a];
        bounced = true;
      } else if (b.position[a] < -3.0f) {
        b.position[a] = -3.0f;
        b.velocity[a] = -b.velocity[a];
        bounced = true;
      }
      if (bounced) m_burst.emit(burstParams(), b.position, m_rng);
    }
  }

  m_fountain.emit(fountainParams(), {-2.0f, 0.2f, 0.0f}, m_rng);
  m_smoke.emit(smokeParams(), {2.0f, 0.2f, 0.0f}, m_rng);
  m_editor.emit(m_editorParams, {0.0f, 0.5f, 0.0f}, m_rng);

  m_burst.update(dt);
  m_fountain.update(dt);
  m_smoke.update(dt);
  m_editor.update(dt);
}

void ParticlesGame::onRender(engine::Renderer& renderer, int width,
                             int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    m_groundMat = renderer.registry().registerMaterial(
        {{0.15f, 0.15f, 0.2f, 1.0f}, 0.0f, 0.9f, s});
    m_bouncerMat = renderer.registry().registerMaterial(
        {{0.8f, 0.8f, 0.85f, 1.0f}, 0.2f, 0.4f, s});

    const engine::MeshHandle cubeMesh = renderer.unitCubeMesh();
    m_ground.addComponent<engine::MeshComponent>(
        engine::MeshComponent{cubeMesh, m_groundMat});
    for (engine::SceneObject& b : m_bouncerObjs)
      b.addComponent<engine::MeshComponent>(
          engine::MeshComponent{cubeMesh, m_bouncerMat});

    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;

  for (size_t i = 0; i < m_bouncerObjs.size(); ++i)
    m_bouncerObjs[i].getComponent<engine::TransformComponent>().translation =
        m_bouncers[i].position;

  m_scene.render(renderer, aspect);

  std::vector<engine::ParticleInstance> instances;
  const auto submit = [&](const engine::ParticleSystem& sys) {
    for (const engine::Particle& p : sys.particles()) {
      instances.push_back(
          {p.position, p.size, glm::vec4(engine::renderColor(p), 1.0f)});
    }
  };
  submit(m_burst);
  submit(m_fountain);
  submit(m_smoke);
  submit(m_editor);
  renderer.submitParticles(instances);
}

void ParticlesGame::onImGuiRender() {
  ImGui::SetNextWindowPos(ImVec2(20, 250), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(340, 320), ImGuiCond_FirstUseEver);
  ImGui::Begin("Particle Editor");
  const char* names[kPresetCount];
  for (int i = 0; i < kPresetCount; ++i) names[i] = kPresets[i].name;
  if (ImGui::Combo("Type", &m_selectedPreset, names, kPresetCount)) {
    m_editorParams = kPresets[m_selectedPreset].make();
  }
  ImGui::SliderInt("Count", &m_editorParams.count, 0, 100);
  ImGui::SliderFloat("Speed min", &m_editorParams.speedMin, 0.0f, 12.0f);
  ImGui::SliderFloat("Speed max", &m_editorParams.speedMax, 0.0f, 12.0f);
  ImGui::SliderFloat("Spread", &m_editorParams.spread, 0.0f, 1.0f);
  ImGui::ColorEdit3("Color", &m_editorParams.color.x);
  ImGui::SliderFloat("Size min", &m_editorParams.sizeMin, 0.01f, 0.5f);
  ImGui::SliderFloat("Size max", &m_editorParams.sizeMax, 0.01f, 0.5f);
  ImGui::SliderFloat("Life min", &m_editorParams.lifeMin, 0.1f, 3.0f);
  ImGui::SliderFloat("Life max", &m_editorParams.lifeMax, 0.1f, 3.0f);
  ImGui::SliderFloat3("Gravity", &m_editorParams.gravity.x, -10.0f, 10.0f);
  ImGui::Text("Live particles: %d", static_cast<int>(m_editor.size()));
  ImGui::End();
}

}  // namespace particles
