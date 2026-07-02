#ifndef ENGINE_PARTICLES_PARTICLESYSTEM_HPP
#define ENGINE_PARTICLES_PARTICLESYSTEM_HPP

#include <cstddef>
#include <glm/glm.hpp>
#include <random>
#include <vector>

#include "engine/particles/Particle.hpp"

namespace engine {

struct EmitParams {
  int count = 0;
  float speedMin = 0.0f;
  float speedMax = 0.0f;
  glm::vec3 direction{0.0f};  // base dir; length 0 => fully radial
  float spread = 1.0f;        // jitter added to the base direction
  glm::vec3 color{1.0f};
  float sizeMin = 0.1f;
  float sizeMax = 0.1f;
  float lifeMin = 1.0f;
  float lifeMax = 1.0f;
  glm::vec3 gravity{0.0f};
};

class ParticleSystem {
 public:
  void emit(const EmitParams& p, const glm::vec3& origin, std::mt19937& rng);
  void update(float dt);
  const std::vector<Particle>& particles() const { return m_particles; }
  std::size_t size() const { return m_particles.size(); }
  void clear() { m_particles.clear(); }

 private:
  std::vector<Particle> m_particles;
};

}  // namespace engine

#endif  // ENGINE_PARTICLES_PARTICLESYSTEM_HPP
