#include "games/shooter/ShooterGame.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "engine/assets/MeshBounds.hpp"
#include "engine/core/AssetPath.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "games/shooter/Components.hpp"

namespace shooter {

void ShooterGame::onAttach() {
  m_camera.setTarget({0.0f, 0.5f, 0.0f});
  m_camera.setOrbit(0.0f, 62.0f, 22.0f);

  const entt::entity player = m_registry.create();
  m_registry.emplace<Transform>(player, glm::vec3(0.0f, 0.4f, 0.0f), 0.6f,
                                0.0f);
  m_registry.emplace<Velocity>(player, glm::vec3(0.0f));
  m_registry.emplace<Player>(player);
}

void ShooterGame::onUpdate(float dt) {
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

void ShooterGame::spawnEnemy() {}  // filled in Task 4

void ShooterGame::stepSim(float /*dt*/) {}  // filled in Tasks 3-5

void ShooterGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_resourcesReady) {
    const engine::ShaderHandle s = renderer.meshShader();
    engine::ResourceRegistry& reg = renderer.registry();
    m_groundMat =
        reg.registerMaterial({{0.05f, 0.06f, 0.09f, 1.0f}, 0.0f, 0.9f, s});
    m_playerModel = engine::loadModel(
        engine::assetPath("asteroid-game/Ships/Viper.obj"), reg, s);
    m_enemyModels[0] =
        engine::loadModel(engine::assetPath("SpaceGame/Spaceship.obj"), reg, s);
    m_enemyModels[1] = engine::loadModel(
        engine::assetPath("SpaceGame/Spaceship3.obj"), reg, s);
    m_enemyModels[2] = engine::loadModel(
        engine::assetPath("asteroid-game/Ships/eliteship.obj"), reg, s);
    m_bulletModel = engine::loadModel(
        engine::assetPath("asteroid-game/Objects/Projectile.obj"), reg, s);
    m_font = engine::Font::Load(
        std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf", 32);
    m_resourcesReady = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_camera.resize(aspect);
  renderer.setCamera(m_camera.camera());

  std::vector<engine::PointLight> lights;
  engine::PointLight key;
  key.positionRadius = glm::vec4(0.0f, 8.0f, 4.0f, 40.0f);
  key.color = glm::vec4(1.0f, 0.97f, 0.9f, 3.0f);
  lights.push_back(key);
  renderer.setPointLights(lights);

  const engine::MeshHandle cube = renderer.unitCubeMesh();
  engine::MeshRenderer meshes(renderer.queue(), renderer.registry(),
                              m_camera.camera());

  glm::mat4 ground = glm::translate(glm::mat4(1.0f), {0.0f, -0.2f, 0.0f});
  ground = glm::scale(ground, {28.0f, 0.05f, 28.0f});
  meshes.submit(cube, m_groundMat, ground);

  const auto drawModel = [&](const engine::Model& model, const glm::vec3& pos,
                             float yaw, float scale) {
    if (!model.valid) return;
    glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
    m = glm::rotate(m, yaw, {0.0f, 1.0f, 0.0f});
    m = glm::scale(m, glm::vec3(scale));
    m = m * engine::fitToUnitTransform(model.bounds);
    for (const engine::Submesh& sm : model.submeshes) {
      meshes.submit(sm.mesh, sm.material, m);
    }
  };

  for (const entt::entity e : m_registry.view<Transform, Player>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_playerModel, t.position, t.yaw, t.scale);
  }
  for (const entt::entity e : m_registry.view<Transform, Enemy>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_enemyModels[m_registry.get<Enemy>(e).tier], t.position, t.yaw,
              t.scale);
  }
  for (const entt::entity e : m_registry.view<Transform, Bullet>()) {
    const Transform& t = m_registry.get<Transform>(e);
    drawModel(m_bulletModel, t.position, t.yaw, t.scale);
  }
}

}  // namespace shooter
