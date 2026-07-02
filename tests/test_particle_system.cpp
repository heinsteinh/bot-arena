#include <catch2/catch_test_macros.hpp>
#include <random>

#include "engine/particles/ParticleSystem.hpp"

using engine::EmitParams;
using engine::ParticleSystem;

TEST_CASE("emit adds count particles within life range", "[particles]") {
  ParticleSystem sys;
  std::mt19937 rng(42);
  EmitParams p;
  p.count = 20;
  p.speedMin = 1.0f;
  p.speedMax = 2.0f;
  p.lifeMin = 0.5f;
  p.lifeMax = 1.5f;
  p.sizeMin = 0.1f;
  p.sizeMax = 0.2f;
  sys.emit(p, {0, 0, 0}, rng);
  REQUIRE(sys.size() == 20);
  for (const auto& q : sys.particles()) {
    REQUIRE(q.life >= 0.5f);
    REQUIRE(q.life <= 1.5f);
    REQUIRE(q.maxLife == q.life);
  }
}

TEST_CASE("update ages particles and removes the dead", "[particles]") {
  ParticleSystem sys;
  std::mt19937 rng(1);
  EmitParams p;
  p.count = 10;
  p.speedMin = 0.0f;
  p.speedMax = 0.0f;
  p.lifeMin = 0.1f;
  p.lifeMax = 0.1f;
  sys.emit(p, {0, 0, 0}, rng);
  REQUIRE(sys.size() == 10);
  sys.update(0.2f);  // past their 0.1s life
  REQUIRE(sys.size() == 0);
}

TEST_CASE("update keeps and moves live particles", "[particles]") {
  ParticleSystem sys;
  std::mt19937 rng(1);
  EmitParams p;
  p.count = 1;
  p.speedMin = 1.0f;
  p.speedMax = 1.0f;
  p.lifeMin = 5.0f;
  p.lifeMax = 5.0f;
  sys.emit(p, {0, 0, 0}, rng);
  sys.update(0.1f);
  REQUIRE(sys.size() == 1);
  const glm::vec3 pos = sys.particles()[0].position;
  REQUIRE(glm::length(pos) > 0.0f);  // moved from the origin
}
