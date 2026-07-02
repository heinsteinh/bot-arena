#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

#include "engine/particles/Particle.hpp"

using engine::integrateParticle;
using engine::isDead;
using engine::lifeFraction;
using engine::Particle;
using engine::renderColor;

TEST_CASE("integrateParticle applies gravity, moves, and ages", "[particle]") {
  Particle p;
  p.position = {0, 0, 0};
  p.velocity = {1, 0, 0};
  p.life = 1.0f;
  p.maxLife = 1.0f;
  const Particle q = integrateParticle(p, 0.5f, {0, -10, 0});
  REQUIRE(q.velocity.y == Catch::Approx(-5.0f));  // gravity*dt
  REQUIRE(q.position.x == Catch::Approx(0.5f));   // vel.x*dt
  REQUIRE(q.position.y == Catch::Approx(-2.5f));  // (vel.y after gravity)*dt
  REQUIRE(q.life == Catch::Approx(0.5f));
}

TEST_CASE("lifeFraction and renderColor fade over life", "[particle]") {
  Particle p;
  p.color = {1, 1, 1};
  p.maxLife = 2.0f;
  p.life = 1.0f;
  REQUIRE(lifeFraction(p) == Catch::Approx(0.5f));
  REQUIRE(renderColor(p).x == Catch::Approx(0.5f));
  p.life = 0.0f;
  REQUIRE(lifeFraction(p) == Catch::Approx(0.0f));
  REQUIRE(isDead(p));
}
