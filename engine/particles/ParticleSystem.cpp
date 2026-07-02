#include "engine/particles/ParticleSystem.hpp"

#include <algorithm>

namespace engine {

namespace {
glm::vec3 randUnit(std::mt19937& rng) {
  std::uniform_real_distribution<float> d(-1.0f, 1.0f);
  glm::vec3 v(d(rng), d(rng), d(rng));
  const float len = glm::length(v);
  return len > 1e-4f ? v / len : glm::vec3(0.0f, 1.0f, 0.0f);
}
}  // namespace

void ParticleSystem::emit(const EmitParams& p, const glm::vec3& origin,
                          std::mt19937& rng) {
  std::uniform_real_distribution<float> speedD(p.speedMin, p.speedMax);
  std::uniform_real_distribution<float> sizeD(p.sizeMin, p.sizeMax);
  std::uniform_real_distribution<float> lifeD(p.lifeMin, p.lifeMax);
  for (int i = 0; i < p.count; ++i) {
    glm::vec3 dir = p.direction + p.spread * randUnit(rng);
    const float len = glm::length(dir);
    dir = len > 1e-4f ? dir / len : randUnit(rng);
    Particle q;
    q.position = origin;
    q.velocity = dir * speedD(rng);
    q.color = p.color;
    q.gravity = p.gravity;
    q.size = sizeD(rng);
    q.life = lifeD(rng);
    q.maxLife = q.life;
    m_particles.push_back(q);
  }
}

void ParticleSystem::update(float dt) {
  for (Particle& q : m_particles) q = integrateParticle(q, dt, q.gravity);
  m_particles.erase(
      std::remove_if(m_particles.begin(), m_particles.end(), isDead),
      m_particles.end());
}

}  // namespace engine
