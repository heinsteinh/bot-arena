#include "engine/assets/Model.hpp"
#include "engine/renderer/MatrixCamera.hpp"
#include "engine/renderer/MeshRenderer.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightCollection.hpp"
#include "engine/scene/MeshComponent.hpp"
#include "engine/scene/ModelComponent.hpp"
#include "engine/scene/ModelTransform.hpp"
#include "engine/scene/Scene.hpp"

namespace engine {

void Scene::render(Renderer& renderer, float aspect) {
  const CameraUniforms cu = cameraUniforms(aspect);
  renderer.setCamera(cu);
  const CollectedLights cl = collectLights(m_registry);
  if (!cl.points.empty()) renderer.setPointLights(cl.points);
  if (cl.hasDirectional) renderer.setLightDirection(cl.directionalDir);
  MatrixCamera cam(cu.view, cu.projection);
  MeshRenderer meshes(renderer.queue(), renderer.registry(), cam);
  auto view = m_registry.view<TransformComponent, MeshComponent>();
  for (const entt::entity e : view) {
    const TransformComponent& t = view.get<TransformComponent>(e);
    const MeshComponent& m = view.get<MeshComponent>(e);
    meshes.submit(m.mesh, m.material, t.localTransform());
  }

  auto modelView = m_registry.view<TransformComponent, ModelComponent>();
  for (const entt::entity e : modelView) {
    const TransformComponent& t = modelView.get<TransformComponent>(e);
    const ModelComponent& mc = modelView.get<ModelComponent>(e);
    const Model& model = renderer.registry().model(mc.model);
    const glm::mat4 m = modelRenderTransform(t, mc, model.bounds);
    for (const Submesh& sm : model.submeshes) {
      const MaterialHandle mat =
          mc.materialOverride != 0 ? mc.materialOverride : sm.material;
      meshes.submit(sm.mesh, mat, m);
    }
  }
}

}  // namespace engine
