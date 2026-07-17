#ifndef ENGINE_SCENE_SCENEOBJECT_HPP
#define ENGINE_SCENE_SCENEOBJECT_HPP

#include <cassert>
#include <entt/entt.hpp>
#include <utility>

#include "engine/scene/Scene.hpp"

namespace engine {

// Non-owning handle over an entt::entity in a Scene's registry. Stores no
// component data itself; forwards to the owning Scene.
class SceneObject {
 public:
  SceneObject() = default;
  SceneObject(entt::entity handle, Scene* scene)
      : m_handle(handle), m_scene(scene) {}

  template <class T, class... Args>
  T& addComponent(Args&&... args) {
    assert(m_scene && !hasComponent<T>());
    return m_scene->m_registry.emplace<T>(m_handle,
                                          std::forward<Args>(args)...);
  }

  template <class T>
  T& getComponent() {
    assert(hasComponent<T>());
    return m_scene->m_registry.get<T>(m_handle);
  }

  template <class T>
  const T& getComponent() const {
    assert(hasComponent<T>());
    return m_scene->m_registry.get<T>(m_handle);
  }

  template <class T>
  bool hasComponent() const {
    return m_scene != nullptr && m_scene->m_registry.all_of<T>(m_handle);
  }

  template <class T>
  void removeComponent() {
    assert(hasComponent<T>());
    m_scene->m_registry.remove<T>(m_handle);
  }

  explicit operator bool() const {
    return m_scene != nullptr && m_scene->m_registry.valid(m_handle);
  }
  operator entt::entity() const { return m_handle; }
  bool operator==(const SceneObject& o) const {
    return m_handle == o.m_handle && m_scene == o.m_scene;
  }
  bool operator!=(const SceneObject& o) const { return !(*this == o); }

 private:
  entt::entity m_handle{entt::null};
  Scene* m_scene = nullptr;
};

}  // namespace engine

#endif  // ENGINE_SCENE_SCENEOBJECT_HPP
