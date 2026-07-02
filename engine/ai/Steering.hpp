#ifndef ENGINE_AI_STEERING_HPP
#define ENGINE_AI_STEERING_HPP

#include <glm/glm.hpp>

namespace engine {

// Clamp a vector's length to `maxLen`.
inline glm::vec3 truncate(glm::vec3 v, float maxLen) {
  const float len = glm::length(v);
  if (len > maxLen && len > 1e-6f) return v * (maxLen / len);
  return v;
}

// Steering force toward `target`: desired = dir*maxSpeed, steering = desired -
// vel (capped at maxForce). Zero at the target.
inline glm::vec3 seek(glm::vec3 pos, glm::vec3 vel, glm::vec3 target,
                      float maxSpeed, float maxForce) {
  const glm::vec3 toTarget = target - pos;
  const float d = glm::length(toTarget);
  if (d < 1e-6f) return glm::vec3(0.0f);
  const glm::vec3 desired = (toTarget / d) * maxSpeed;
  return truncate(desired - vel, maxForce);
}

// Steering force directly away from `target`.
inline glm::vec3 flee(glm::vec3 pos, glm::vec3 vel, glm::vec3 target,
                      float maxSpeed, float maxForce) {
  const glm::vec3 away = pos - target;
  const float d = glm::length(away);
  if (d < 1e-6f) return glm::vec3(0.0f);
  const glm::vec3 desired = (away / d) * maxSpeed;
  return truncate(desired - vel, maxForce);
}

}  // namespace engine

#endif  // ENGINE_AI_STEERING_HPP
