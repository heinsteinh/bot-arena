#ifndef GAMES_PARTICLES_PARTICLESGAME_HPP
#define GAMES_PARTICLES_PARTICLESGAME_HPP

#include <random>
#include <vector>

#include "engine/core/Layer.hpp"
#include "engine/particles/ParticleSystem.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/RenderCommand.hpp"

namespace particles {

class ParticlesGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void stepSim(float dt);

  engine::OrbitCameraController m_camera;
  std::mt19937 m_rng{7};
  float m_accumulator = 0.0f;
  bool m_resourcesReady = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_bouncerMat = 0;

  struct Bouncer {
    glm::vec3 position;
    glm::vec3 velocity;
  };
  std::vector<Bouncer> m_bouncers;
  engine::ParticleSystem m_burst;
  engine::ParticleSystem m_fountain;
  engine::ParticleSystem m_smoke;
};

}  // namespace particles

#endif  // GAMES_PARTICLES_PARTICLESGAME_HPP
