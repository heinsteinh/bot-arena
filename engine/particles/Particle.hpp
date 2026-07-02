#ifndef ENGINE_PARTICLES_PARTICLE_HPP
#define ENGINE_PARTICLES_PARTICLE_HPP

#include <algorithm>
#include <glm/glm.hpp>

namespace engine {

struct Particle {
  glm::vec3 position{0.0f};
  glm::vec3 velocity{0.0f};
  glm::vec3 color{1.0f};
  glm::vec3 gravity{0.0f};
  float size = 0.1f;
  float life = 0.0f;
  float maxLife = 1.0f;
};

inline Particle integrateParticle(Particle p, float dt, glm::vec3 gravity) {
  p.velocity += gravity * dt;
  p.position += p.velocity * dt;
  p.life -= dt;
  return p;
}

inline float lifeFraction(const Particle& p) {
  if (p.maxLife <= 0.0f) return 0.0f;
  return std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
}

inline glm::vec3 renderColor(const Particle& p) {
  return p.color * lifeFraction(p);  // fades to black as it dies
}

inline bool isDead(const Particle& p) { return p.life <= 0.0f; }

}  // namespace engine

#endif  // ENGINE_PARTICLES_PARTICLE_HPP
