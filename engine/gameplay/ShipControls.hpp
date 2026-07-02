#ifndef ENGINE_GAMEPLAY_SHIPCONTROLS_HPP
#define ENGINE_GAMEPLAY_SHIPCONTROLS_HPP

#include <cmath>
#include <glm/glm.hpp>

namespace engine {

// Yaw (radians about +Y) so a +Z-forward model points along v in the XZ plane.
inline float headingToYaw(const glm::vec3& v) { return std::atan2(v.x, v.z); }

// Unit forward vector for a yaw (inverse of headingToYaw on XZ).
inline glm::vec3 forwardFromYaw(float yaw) {
  return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw));
}

// True when two circles on the XZ plane overlap (Y ignored).
inline bool circlesOverlapXZ(const glm::vec3& a, float ra, const glm::vec3& b,
                             float rb) {
  const float dx = a.x - b.x;
  const float dz = a.z - b.z;
  const float r = ra + rb;
  return dx * dx + dz * dz < r * r;
}

}  // namespace engine

#endif  // ENGINE_GAMEPLAY_SHIPCONTROLS_HPP
