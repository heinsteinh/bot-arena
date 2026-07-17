#include "engine/renderer/MatrixCamera.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/Scene.hpp"

namespace engine {

void Scene::render(Renderer& renderer, float aspect) {
  const CameraUniforms cu = cameraUniforms(aspect);
  renderer.setCamera(cu);
  MatrixCamera cam(cu.view, cu.projection);
  MeshRenderer meshes(renderer.queue(), renderer.registry(), cam);
  auto view = m_registry.view<TransformComponent, MeshComponent>();
  for (const entt::entity e : view) {
    const TransformComponent& t = view.get<TransformComponent>(e);
    const MeshComponent& m = view.get<MeshComponent>(e);
    meshes.submit(m.mesh, m.material, t.localTransform());
  }
}

}  // namespace engine
