#ifndef ENGINE_SCENE_SCENECAMERA_HPP
#define ENGINE_SCENE_SCENECAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/scene/Components.hpp"

namespace engine {

// View = inverse of the camera's world transform (translation + rotation only;
// scale is intentionally excluded from a view matrix).
inline glm::mat4 viewMatrix(const TransformComponent& t) {
  const glm::mat4 world = glm::translate(glm::mat4(1.0f), t.translation) *
                          glm::mat4_cast(t.rotation);
  return glm::inverse(world);
}

inline glm::mat4 projectionMatrix(const CameraComponent& c, float aspect) {
  const float a = c.fixedAspectRatio ? c.aspect : aspect;
  if (c.type == ProjectionType::Perspective) {
    return glm::perspective(glm::radians(c.fov), a, c.perspNear, c.perspFar);
  }
  const float h = c.orthoSize * 0.5f;
  const float w = h * a;
  return glm::ortho(-w, w, -h, h, c.orthoNear, c.orthoFar);
}

}  // namespace engine

#endif  // ENGINE_SCENE_SCENECAMERA_HPP
