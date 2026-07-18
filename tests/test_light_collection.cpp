#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "engine/scene/Components.hpp"
#include "engine/scene/LightCollection.hpp"
#include "engine/scene/LightComponent.hpp"

using engine::CollectedLights;
using engine::LightComponent;
using engine::LightType;
using engine::TransformComponent;

namespace {

entt::entity makeLight(entt::registry& reg, const glm::vec3& pos,
                       const LightComponent& lc) {
  const entt::entity e = reg.create();
  TransformComponent t;
  t.translation = pos;
  reg.emplace<TransformComponent>(e, t);
  reg.emplace<LightComponent>(e, lc);
  return e;
}

}  // namespace

TEST_CASE("collectLights: empty registry yields nothing") {
  entt::registry reg;
  const CollectedLights cl = engine::collectLights(reg);
  CHECK(cl.points.empty());
  CHECK_FALSE(cl.hasDirectional);
}

TEST_CASE(
    "collectLights: point light maps translation/radius/color/intensity") {
  entt::registry reg;
  LightComponent lc;
  lc.type = LightType::Point;
  lc.color = glm::vec3(0.2f, 0.4f, 0.6f);
  lc.intensity = 3.0f;
  lc.radius = 8.0f;
  makeLight(reg, glm::vec3(1.0f, 2.0f, 3.0f), lc);

  const CollectedLights cl = engine::collectLights(reg);
  REQUIRE(cl.points.size() == 1);
  CHECK(cl.points[0].positionRadius.x == Catch::Approx(1.0f));
  CHECK(cl.points[0].positionRadius.y == Catch::Approx(2.0f));
  CHECK(cl.points[0].positionRadius.z == Catch::Approx(3.0f));
  CHECK(cl.points[0].positionRadius.w == Catch::Approx(8.0f));
  CHECK(cl.points[0].color.r == Catch::Approx(0.2f));
  CHECK(cl.points[0].color.g == Catch::Approx(0.4f));
  CHECK(cl.points[0].color.b == Catch::Approx(0.6f));
  CHECK(cl.points[0].color.a == Catch::Approx(3.0f));
  CHECK_FALSE(cl.hasDirectional);
}

TEST_CASE("collectLights: directional dir is normalized translation") {
  entt::registry reg;
  LightComponent lc;
  lc.type = LightType::Directional;
  makeLight(reg, glm::vec3(0.5f, 0.7f, 0.35f), lc);

  const CollectedLights cl = engine::collectLights(reg);
  CHECK(cl.points.empty());
  REQUIRE(cl.hasDirectional);
  const glm::vec3 expect = glm::normalize(glm::vec3(0.5f, 0.7f, 0.35f));
  CHECK(cl.directionalDir.x == Catch::Approx(expect.x));
  CHECK(cl.directionalDir.y == Catch::Approx(expect.y));
  CHECK(cl.directionalDir.z == Catch::Approx(expect.z));
}

TEST_CASE("collectLights: first directional wins, later ignored") {
  entt::registry reg;
  LightComponent a;
  a.type = LightType::Directional;
  const entt::entity first = makeLight(reg, glm::vec3(1.0f, 0.0f, 0.0f), a);
  LightComponent b;
  b.type = LightType::Directional;
  makeLight(reg, glm::vec3(0.0f, 1.0f, 0.0f), b);

  const CollectedLights cl = engine::collectLights(reg);
  REQUIRE(cl.hasDirectional);
  // Whichever entity the view visits first supplies the direction; assert the
  // result equals exactly one of the two candidates, never a blend, and that
  // no point lights were produced.
  CHECK(cl.points.empty());
  const glm::vec3 dir = cl.directionalDir;
  const bool isX = dir.x == Catch::Approx(1.0f) && dir.y == Catch::Approx(0.0f);
  const bool isY = dir.y == Catch::Approx(1.0f) && dir.x == Catch::Approx(0.0f);
  CHECK((isX || isY));
  (void)first;
}

TEST_CASE("collectLights: mixed scene yields N points and one directional") {
  entt::registry reg;
  LightComponent p;
  p.type = LightType::Point;
  makeLight(reg, glm::vec3(1.0f, 0.0f, 0.0f), p);
  makeLight(reg, glm::vec3(2.0f, 0.0f, 0.0f), p);
  LightComponent d;
  d.type = LightType::Directional;
  makeLight(reg, glm::vec3(0.0f, 1.0f, 0.0f), d);

  const CollectedLights cl = engine::collectLights(reg);
  CHECK(cl.points.size() == 2);
  CHECK(cl.hasDirectional);
}
