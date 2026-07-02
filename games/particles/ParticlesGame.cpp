#include "games/particles/ParticlesGame.hpp"

#include <imgui.h>

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "engine/particles/Particle.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/ParticleInstance.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

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
}  // namespace

void ParticlesGame::onAttach() {
  m_camera.setTarget({0.0f, 1.0f, 0.0f});
  m_camera.setOrbit(40.0f, 25.0f, 14.0f);

  std::uniform_real_distribution<float> posD(-2.0f, 2.0f);
  std::uniform_real_distribution<float> velD(-3.5f, 3.5f);
  for (int i = 0; i < 6; ++i) {
    m_bouncers.push_back(
        {{posD(m_rng), 1.0f, posD(m_rng)}, {velD(m_rng), 0.0f, velD(m_rng)}});
  }

  for (int i = 0; i < 90; ++i) stepSim(1.0f / 60.0f);
}

void ParticlesGame::onUpdate(float dt) {
  m_camera.update(dt);
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

  m_burst.update(dt);
  m_fountain.update(dt);
  m_smoke.update(dt);
}

void ParticlesGame::onRender(engine::Renderer& renderer, int width,
                             int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    m_groundMat = renderer.registry().registerMaterial(
        {{0.15f, 0.15f, 0.2f, 1.0f}, 0.0f, 0.9f, s});
    m_bouncerMat = renderer.registry().registerMaterial(
        {{0.8f, 0.8f, 0.85f, 1.0f}, 0.2f, 0.4f, s});
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_camera.resize(aspect);
  renderer.setCamera(m_camera.camera());

  std::vector<engine::PointLight> lights;
  engine::PointLight key;
  key.positionRadius = glm::vec4(0.0f, 6.0f, 0.0f, 20.0f);
  key.color = glm::vec4(1.0f, 1.0f, 1.0f, 2.0f);
  lights.push_back(key);
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(),
                              m_camera.camera());

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.05f, 0.0f});
  ground = glm::scale(ground, {20.0f, 0.05f, 20.0f});
  meshes.submit(cube, m_groundMat, ground);

  for (const Bouncer& b : m_bouncers) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), b.position);
    m = glm::scale(m, glm::vec3(0.3f));
    meshes.submit(cube, m_bouncerMat, m);
  }

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
  renderer.submitParticles(instances);
}

void ParticlesGame::onImGuiRender() {
  ImGui::SetNextWindowPos(ImVec2(20, 250), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(320, 130), ImGuiCond_FirstUseEver);
  ImGui::Begin("Particle Editor");
  ImGui::Text("ImGui is live.");
  ImGui::End();
}

}  // namespace particles
