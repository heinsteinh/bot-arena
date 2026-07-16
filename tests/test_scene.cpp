#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/scene/Components.hpp"
#include "engine/scene/SceneCamera.hpp"

namespace {
void requireMat4Eq(const glm::mat4& a, const glm::mat4& b) {
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      REQUIRE(a[c][r] == Catch::Approx(b[c][r]).margin(1e-4));
}
}  // namespace

TEST_CASE("TransformComponent localTransform is T*R*S", "[scene]") {
  engine::TransformComponent t;
  t.translation = {1.0f, 2.0f, 3.0f};
  t.rotation = {glm::radians(10.0f), glm::radians(20.0f), 0.0f};
  t.scale = {2.0f, 0.5f, 1.0f};
  const glm::mat4 expected = glm::translate(glm::mat4(1.0f), t.translation) *
                             glm::mat4_cast(glm::quat(t.rotation)) *
                             glm::scale(glm::mat4(1.0f), t.scale);
  requireMat4Eq(t.localTransform(), expected);
}

TEST_CASE("viewMatrix is inverse of translate*rotate", "[scene]") {
  engine::TransformComponent t;
  t.translation = {0.0f, 4.0f, 9.0f};
  t.rotation = {glm::radians(-24.0f), 0.0f, 0.0f};
  const glm::mat4 world = glm::translate(glm::mat4(1.0f), t.translation) *
                          glm::mat4_cast(glm::quat(t.rotation));
  requireMat4Eq(engine::viewMatrix(t), glm::inverse(world));
  // camera position = inverse(view) translation column
  const glm::vec3 pos = glm::vec3(glm::inverse(engine::viewMatrix(t))[3]);
  REQUIRE(pos.x == Catch::Approx(t.translation.x).margin(1e-4));
  REQUIRE(pos.y == Catch::Approx(t.translation.y).margin(1e-4));
  REQUIRE(pos.z == Catch::Approx(t.translation.z).margin(1e-4));
}

TEST_CASE("projectionMatrix perspective matches glm::perspective", "[scene]") {
  engine::CameraComponent c;
  c.type = engine::ProjectionType::Perspective;
  c.fov = 55.0f;
  c.perspNear = 0.1f;
  c.perspFar = 100.0f;
  requireMat4Eq(engine::projectionMatrix(c, 1.6f),
                glm::perspective(glm::radians(55.0f), 1.6f, 0.1f, 100.0f));
}

TEST_CASE("projectionMatrix orthographic matches glm::ortho", "[scene]") {
  engine::CameraComponent c;
  c.type = engine::ProjectionType::Orthographic;
  c.orthoSize = 10.0f;
  c.orthoNear = -1.0f;
  c.orthoFar = 1.0f;
  const float aspect = 1.6f;
  const float h = 5.0f;
  const float w = h * aspect;
  requireMat4Eq(engine::projectionMatrix(c, aspect),
                glm::ortho(-w, w, -h, h, -1.0f, 1.0f));
}

TEST_CASE("fixedAspectRatio uses the component aspect", "[scene]") {
  engine::CameraComponent c;
  c.fixedAspectRatio = true;
  c.aspect = 2.0f;
  requireMat4Eq(
      engine::projectionMatrix(c, 999.0f),
      glm::perspective(glm::radians(c.fov), 2.0f, c.perspNear, c.perspFar));
}
