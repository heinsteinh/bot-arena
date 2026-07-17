#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/scene/Components.hpp"
#include "engine/scene/Scene.hpp"
#include "engine/scene/SceneCamera.hpp"
#include "engine/scene/SceneObject.hpp"

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
  t.rotation =
      glm::quat(glm::vec3(glm::radians(10.0f), glm::radians(20.0f), 0.0f));
  t.scale = {2.0f, 0.5f, 1.0f};
  const glm::mat4 expected = glm::translate(glm::mat4(1.0f), t.translation) *
                             glm::mat4_cast(t.rotation) *
                             glm::scale(glm::mat4(1.0f), t.scale);
  requireMat4Eq(t.localTransform(), expected);

  // Independent oracle: 90 deg about +Y rotates +X to -Z (scale x=2 -> length
  // 2).
  engine::TransformComponent r;
  r.rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::vec4 x = r.localTransform() * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
  REQUIRE(x.x == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(x.y == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(x.z == Catch::Approx(-1.0f).margin(1e-4));
}

TEST_CASE("viewMatrix is inverse of translate*rotate", "[scene]") {
  engine::TransformComponent t;
  t.translation = {0.0f, 4.0f, 9.0f};
  t.rotation = glm::quat(glm::vec3(glm::radians(-24.0f), 0.0f, 0.0f));
  const glm::mat4 world = glm::translate(glm::mat4(1.0f), t.translation) *
                          glm::mat4_cast(t.rotation);
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

TEST_CASE("createObject auto-adds ID, Tag, Transform", "[scene]") {
  engine::Scene scene;
  engine::SceneObject o = scene.createObject("Hero");
  REQUIRE(static_cast<bool>(o));
  REQUIRE(o.hasComponent<engine::IDComponent>());
  REQUIRE(o.hasComponent<engine::TagComponent>());
  REQUIRE(o.hasComponent<engine::TransformComponent>());
  REQUIRE(o.getComponent<engine::TagComponent>().name == "Hero");
}

TEST_CASE("IDs are unique and increasing; default name", "[scene]") {
  engine::Scene scene;
  engine::SceneObject a = scene.createObject();
  engine::SceneObject b = scene.createObject();
  REQUIRE(a.getComponent<engine::IDComponent>().id == 1u);
  REQUIRE(b.getComponent<engine::IDComponent>().id == 2u);
  REQUIRE(a.getComponent<engine::TagComponent>().name == "SceneObject");
}

TEST_CASE("add/get/remove component round-trip", "[scene]") {
  engine::Scene scene;
  engine::SceneObject o = scene.createObject();
  REQUIRE_FALSE(o.hasComponent<engine::CameraComponent>());
  engine::CameraComponent& c = o.addComponent<engine::CameraComponent>();
  c.fov = 33.0f;
  REQUIRE(o.hasComponent<engine::CameraComponent>());
  REQUIRE(o.getComponent<engine::CameraComponent>().fov == 33.0f);
  o.removeComponent<engine::CameraComponent>();
  REQUIRE_FALSE(o.hasComponent<engine::CameraComponent>());
}

TEST_CASE("operator bool false for default and destroyed", "[scene]") {
  engine::SceneObject none;
  REQUIRE_FALSE(static_cast<bool>(none));
  engine::Scene scene;
  engine::SceneObject o = scene.createObject();
  REQUIRE(static_cast<bool>(o));
  scene.destroyObject(o);
  REQUIRE_FALSE(static_cast<bool>(o));
}

TEST_CASE("primaryCamera selection", "[scene]") {
  engine::Scene scene;
  REQUIRE_FALSE(static_cast<bool>(scene.primaryCamera()));
  engine::SceneObject a = scene.createObject("A");
  engine::CameraComponent& ca = a.addComponent<engine::CameraComponent>();
  ca.primary = false;
  REQUIRE_FALSE(static_cast<bool>(scene.primaryCamera()));
  engine::SceneObject b = scene.createObject("B");
  b.addComponent<engine::CameraComponent>();  // primary = true (default)
  engine::SceneObject cam = scene.primaryCamera();
  REQUIRE(static_cast<bool>(cam));
  REQUIRE(cam == b);
}

TEST_CASE("cameraUniforms composes view and projection", "[scene]") {
  engine::Scene scene;
  engine::SceneObject cam = scene.createObject("Camera");
  engine::TransformComponent& tf =
      cam.getComponent<engine::TransformComponent>();
  tf.translation = {0.0f, 4.0f, 9.0f};
  tf.rotation = glm::quat(glm::vec3(glm::radians(-24.0f), 0.0f, 0.0f));
  engine::CameraComponent& cc = cam.addComponent<engine::CameraComponent>();
  cc.fov = 55.0f;
  const engine::CameraUniforms u = scene.cameraUniforms(1.6f);
  const engine::CameraUniforms expected = engine::makeCameraUniforms(
      engine::viewMatrix(tf), engine::projectionMatrix(cc, 1.6f));
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      REQUIRE(u.viewProjection[c][r] ==
              Catch::Approx(expected.viewProjection[c][r]).margin(1e-4));
  REQUIRE(u.cameraPosition.y == Catch::Approx(4.0f).margin(1e-4));
}
