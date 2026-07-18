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

// A TransformComponent whose viewMatrix() equals glm::lookAt(eye, target, up).
// world = inverse(lookAt) = translate(eye) * rotate; we extract eye as the
// translation and the rotation as a quaternion. Reproduces a raw lookAt camera
// exactly (the quat_cast round-trip is sub-pixel), so a migrated Scene camera
// matches the original view matrix.
inline TransformComponent lookAtTransform(
    const glm::vec3& eye, const glm::vec3& target,
    const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
  const glm::mat4 world = glm::inverse(glm::lookAt(eye, target, up));
  TransformComponent t;
  t.translation = eye;
  t.rotation = glm::quat_cast(glm::mat3(world));
  return t;
}

// A TransformComponent whose viewMatrix() equals the given (rigid) view matrix.
// world = inverse(view) = translate * rotate; extract translation + rotation.
// The general form of lookAtTransform, for cameras that aren't a clean
// eye/target lookAt (fly, orthographic).
inline TransformComponent cameraTransformFromView(const glm::mat4& view) {
  const glm::mat4 world = glm::inverse(view);
  TransformComponent t;
  t.translation = glm::vec3(world[3]);
  t.rotation = glm::quat_cast(glm::mat3(world));
  return t;
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
