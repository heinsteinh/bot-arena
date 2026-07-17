#ifndef ENGINE_SCENE_SCENE_HPP
#define ENGINE_SCENE_SCENE_HPP

#include <cstdint>
#include <entt/entt.hpp>
#include <string_view>

#include "engine/renderer/CameraUniforms.hpp"

namespace engine {

class SceneObject;

class Scene {
 public:
  Scene() = default;

  // SceneObjects cache a raw Scene*, so a Scene must never move or copy out
  // from under its handles (entt::registry is move-only, which would otherwise
  // make Scene implicitly movable).
  Scene(const Scene&) = delete;
  Scene& operator=(const Scene&) = delete;
  Scene(Scene&&) = delete;
  Scene& operator=(Scene&&) = delete;

  SceneObject createObject(std::string_view name = {});
  void destroyObject(SceneObject object);

  SceneObject primaryCamera();
  CameraUniforms cameraUniforms(float aspect) const;

  void update(float dt);

  entt::registry& registry() { return m_registry; }
  const entt::registry& registry() const { return m_registry; }

 private:
  entt::registry m_registry;
  uint64_t m_nextId = 1;

  friend class SceneObject;
};

}  // namespace engine

#endif  // ENGINE_SCENE_SCENE_HPP
