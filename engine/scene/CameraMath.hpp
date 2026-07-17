#ifndef ENGINE_SCENE_CAMERAMATH_HPP
#define ENGINE_SCENE_CAMERAMATH_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace engine {

// FPS-style orientation: yaw about world +Y, then pitch about local +X.
// yaw = pitch = 0 -> camera looks down -Z.
inline glm::quat orientationFromYawPitch(float yawDeg, float pitchDeg) {
  return glm::angleAxis(glm::radians(yawDeg), glm::vec3(0.0f, 1.0f, 0.0f)) *
         glm::angleAxis(glm::radians(pitchDeg), glm::vec3(1.0f, 0.0f, 0.0f));
}

// The camera forward: the -Z axis rotated by q.
inline glm::vec3 forwardDir(const glm::quat& q) {
  return q * glm::vec3(0.0f, 0.0f, -1.0f);
}

// Orientation whose -Z points along `forward` (look-at), with up hint `up`.
inline glm::quat lookRotation(const glm::vec3& forward,
                              const glm::vec3& up = glm::vec3(0.0f, 1.0f,
                                                              0.0f)) {
  const glm::vec3 f = glm::normalize(forward);
  glm::vec3 r = glm::cross(f, up);
  const float rl = glm::length(r);
  r = rl > 1e-5f ? r / rl
                 : glm::vec3(1.0f, 0.0f, 0.0f);  // forward parallel to up
  const glm::vec3 u = glm::cross(r, f);
  return glm::quat_cast(glm::mat3(r, u, -f));
}

// Camera position orbiting `center` at `distance` along the yaw/pitch
// direction.
inline glm::vec3 orbitPosition(const glm::vec3& center, float yawDeg,
                               float pitchDeg, float distance) {
  return center -
         forwardDir(orientationFromYawPitch(yawDeg, pitchDeg)) * distance;
}

}  // namespace engine

#endif  // ENGINE_SCENE_CAMERAMATH_HPP
