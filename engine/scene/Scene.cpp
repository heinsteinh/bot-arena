#include "engine/scene/Scene.hpp"

#include <string>

#include "engine/scene/Components.hpp"
#include "engine/scene/SceneCamera.hpp"
#include "engine/scene/SceneObject.hpp"

namespace engine {

SceneObject Scene::createObject(std::string_view name) {
  const entt::entity handle = m_registry.create();
  SceneObject object(handle, this);
  object.addComponent<IDComponent>(IDComponent{m_nextId++});
  object.addComponent<TagComponent>(TagComponent{
      name.empty() ? std::string("SceneObject") : std::string(name)});
  object.addComponent<TransformComponent>();
  return object;
}

void Scene::destroyObject(SceneObject object) {
  if (object) m_registry.destroy(static_cast<entt::entity>(object));
}

SceneObject Scene::primaryCamera() {
  auto view = m_registry.view<CameraComponent>();
  for (const entt::entity e : view) {
    if (view.get<CameraComponent>(e).primary) return SceneObject(e, this);
  }
  return SceneObject();
}

CameraUniforms Scene::cameraUniforms(float aspect) const {
  auto view = m_registry.view<const CameraComponent>();
  for (const entt::entity e : view) {
    const CameraComponent& cam = view.get<const CameraComponent>(e);
    if (!cam.primary) continue;
    const TransformComponent& tf = m_registry.get<const TransformComponent>(e);
    return makeCameraUniforms(viewMatrix(tf), projectionMatrix(cam, aspect));
  }
  return makeCameraUniforms(glm::mat4(1.0f), glm::mat4(1.0f));
}

}  // namespace engine
