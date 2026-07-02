#ifndef ENGINE_PHYSICS_COLLISION_HPP
#define ENGINE_PHYSICS_COLLISION_HPP

#include <cmath>
#include <glm/glm.hpp>

namespace engine {

struct WallBounce {
  glm::vec3 position;
  glm::vec3 velocity;
};

// Keep an AABB agent of half-extent `radius` inside [boundsMin, boundsMax] by
// clamping to the wall and reflecting the crossing velocity component inward.
inline WallBounce resolveWallBounce(glm::vec3 pos, glm::vec3 vel,
                                    glm::vec3 boundsMin, glm::vec3 boundsMax,
                                    float radius) {
  for (int a = 0; a < 3; ++a) {
    if (pos[a] + radius > boundsMax[a]) {
      pos[a] = boundsMax[a] - radius;
      vel[a] = -std::abs(vel[a]);
    } else if (pos[a] - radius < boundsMin[a]) {
      pos[a] = boundsMin[a] + radius;
      vel[a] = std::abs(vel[a]);
    }
  }
  return {pos, vel};
}

}  // namespace engine

#endif  // ENGINE_PHYSICS_COLLISION_HPP
