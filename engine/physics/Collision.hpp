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

struct AgentPair {
  glm::vec3 posA;
  glm::vec3 velA;
  glm::vec3 posB;
  glm::vec3 velB;
};

// Circle-vs-circle in XZ: separate an overlapping pair (each pushed half the
// penetration along the horizontal normal) and, if they are approaching, swap
// the normal component of their velocities (equal mass). `y` is untouched.
inline AgentPair resolveAgentPair(glm::vec3 posA, glm::vec3 velA, float rA,
                                  glm::vec3 posB, glm::vec3 velB, float rB) {
  const float dx = posB.x - posA.x;
  const float dz = posB.z - posA.z;
  float dist = std::sqrt(dx * dx + dz * dz);
  const float minDist = rA + rB;
  if (dist >= minDist) return {posA, velA, posB, velB};  // no overlap

  glm::vec3 n;
  if (dist > 1e-4f) {
    n = glm::vec3(dx / dist, 0.0f, dz / dist);
  } else {
    n = glm::vec3(1.0f, 0.0f, 0.0f);  // coincident: pick an axis
    dist = 0.0f;
  }
  const float overlap = minDist - dist;
  posA -= n * (overlap * 0.5f);
  posB += n * (overlap * 0.5f);

  const float vaN = glm::dot(velA, n);
  const float vbN = glm::dot(velB, n);
  if (vaN - vbN > 0.0f) {  // approaching along the normal
    velA += (vbN - vaN) * n;
    velB += (vaN - vbN) * n;
  }
  return {posA, velA, posB, velB};
}

}  // namespace engine

#endif  // ENGINE_PHYSICS_COLLISION_HPP
