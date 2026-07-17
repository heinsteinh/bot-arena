#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/core/Input.hpp"
#include "engine/scene/CameraMath.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ControllerComponents.hpp"
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

TEST_CASE("orientationFromYawPitch conventions", "[cameramath]") {
  const glm::vec3 f0 =
      engine::forwardDir(engine::orientationFromYawPitch(0, 0));
  REQUIRE(f0.x == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(f0.y == Catch::Approx(0.0f).margin(1e-4));
  REQUIRE(f0.z == Catch::Approx(-1.0f).margin(1e-4));
  // +pitch looks up (forward.y > 0); +yaw turns forward off -Z.
  REQUIRE(engine::forwardDir(engine::orientationFromYawPitch(0, 30)).y > 0.0f);
  REQUIRE(engine::forwardDir(engine::orientationFromYawPitch(30, 0)).x !=
          Catch::Approx(0.0f).margin(1e-3));
}

TEST_CASE("lookRotation orients -Z along the direction", "[cameramath]") {
  const glm::vec3 dir = glm::normalize(glm::vec3(1.0f, -2.0f, 3.0f));
  const glm::vec3 f = engine::forwardDir(engine::lookRotation(dir));
  REQUIRE(f.x == Catch::Approx(dir.x).margin(1e-4));
  REQUIRE(f.y == Catch::Approx(dir.y).margin(1e-4));
  REQUIRE(f.z == Catch::Approx(dir.z).margin(1e-4));
}

TEST_CASE("lookRotation view matches glm::lookAt", "[cameramath]") {
  const glm::vec3 eye(2.0f, 3.0f, 5.0f), target(0.0f, 1.0f, 0.0f);
  engine::TransformComponent t;
  t.translation = eye;
  t.rotation = engine::lookRotation(target - eye);
  const glm::mat4 got = engine::viewMatrix(t);
  const glm::mat4 want = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
  for (int c = 0; c < 4; ++c)
    for (int r = 0; r < 4; ++r)
      REQUIRE(got[c][r] == Catch::Approx(want[c][r]).margin(1e-4));
}

TEST_CASE("orbitPosition sits distance from center with positive pitch above",
          "[cameramath]") {
  const glm::vec3 center(0.0f, 1.0f, 0.0f);
  const glm::vec3 pos = engine::orbitPosition(center, 45.0f, 30.0f, 10.0f);
  REQUIRE(glm::length(pos - center) == Catch::Approx(10.0f).margin(1e-4));
  // positive pitch puts the camera ABOVE the target (elevation)
  const glm::vec3 elevated = engine::orbitPosition(center, 0.0f, 30.0f, 10.0f);
  REQUIRE(elevated.y > center.y);
}

TEST_CASE("orbit controller sets pose from params (no input)", "[controller]") {
  engine::Input::beginFrame();
  engine::Scene scene;
  engine::SceneObject cam = scene.createObject("Cam");
  engine::OrbitControllerComponent& oc =
      cam.addComponent<engine::OrbitControllerComponent>();
  oc.targetPoint = {0.0f, 1.0f, 0.0f};
  oc.yaw = 45.0f;
  oc.pitch = 30.0f;
  oc.distance = 10.0f;
  scene.update(0.016f);
  const engine::TransformComponent& t =
      cam.getComponent<engine::TransformComponent>();
  const glm::vec3 center = oc.targetPoint;
  const glm::vec3 want = engine::orbitPosition(center, 45.0f, 30.0f, 10.0f);
  REQUIRE(t.translation.x == Catch::Approx(want.x).margin(1e-4));
  REQUIRE(t.translation.y == Catch::Approx(want.y).margin(1e-4));
  REQUIRE(t.translation.z == Catch::Approx(want.z).margin(1e-4));
  // positive pitch = camera elevated above the target
  REQUIRE(t.translation.y > center.y);
  // rotation looks from the camera toward the orbit center
  const glm::vec3 wantFwd = glm::normalize(center - t.translation);
  const glm::vec3 gotFwd = engine::forwardDir(t.rotation);
  REQUIRE(glm::length(gotFwd - wantFwd) == Catch::Approx(0.0f).margin(1e-3));
}

TEST_CASE("orbit controller left-drag changes yaw", "[controller]") {
  engine::Input::beginFrame();
  engine::Input::setMouseButton(engine::MouseButton::Left, true);
  engine::Input::setMouseDelta(40.0f, 0.0f);
  engine::Scene scene;
  engine::SceneObject cam = scene.createObject("Cam");
  engine::OrbitControllerComponent& oc =
      cam.addComponent<engine::OrbitControllerComponent>();
  const float y0 = oc.yaw;
  scene.update(0.016f);
  REQUIRE(cam.getComponent<engine::OrbitControllerComponent>().yaw ==
          Catch::Approx(y0 + 40.0f * oc.rotateSpeed).margin(1e-3));
  engine::Input::setMouseButton(engine::MouseButton::Left, false);
  engine::Input::beginFrame();
}

TEST_CASE("fly controller W moves along planar forward", "[controller]") {
  engine::Input::beginFrame();
  engine::Input::setKey(engine::Key::W, true);
  engine::Scene scene;
  engine::SceneObject cam = scene.createObject("Cam");
  engine::FlyControllerComponent& fc =
      cam.addComponent<engine::FlyControllerComponent>();
  fc.yaw = 0.0f;
  fc.pitch = 0.0f;
  fc.moveSpeed = 10.0f;
  cam.getComponent<engine::TransformComponent>().translation = {0, 0, 0};
  scene.update(1.0f);
  const glm::vec3 p =
      cam.getComponent<engine::TransformComponent>().translation;
  REQUIRE(p.z == Catch::Approx(-10.0f).margin(1e-3));  // forward at yaw=0 is -Z
  REQUIRE(p.x == Catch::Approx(0.0f).margin(1e-3));
  engine::Input::setKey(engine::Key::W, false);
  engine::Input::beginFrame();
}

TEST_CASE("follow controller sits at target + offset facing target",
          "[controller]") {
  engine::Input::beginFrame();
  engine::Scene scene;
  engine::SceneObject target = scene.createObject("Target");
  target.getComponent<engine::TransformComponent>().translation = {5, 0, -3};
  engine::SceneObject cam = scene.createObject("Cam");
  engine::FollowControllerComponent& fc =
      cam.addComponent<engine::FollowControllerComponent>();
  fc.target = static_cast<entt::entity>(target);
  fc.offset = {0, 4, 9};
  scene.update(0.016f);
  const engine::TransformComponent& t =
      cam.getComponent<engine::TransformComponent>();
  REQUIRE(t.translation.x == Catch::Approx(5.0f).margin(1e-4));
  REQUIRE(t.translation.y == Catch::Approx(4.0f).margin(1e-4));
  REQUIRE(t.translation.z == Catch::Approx(6.0f).margin(1e-4));
  // forward points from camera toward the target
  const glm::vec3 f = engine::forwardDir(t.rotation);
  const glm::vec3 toTarget =
      glm::normalize(glm::vec3(5, 0, -3) - t.translation);
  REQUIRE(f.x == Catch::Approx(toTarget.x).margin(1e-3));
  REQUIRE(f.z == Catch::Approx(toTarget.z).margin(1e-3));
}

TEST_CASE("camera2D pans with keys and zooms with scroll", "[controller]") {
  engine::Input::beginFrame();
  engine::Input::setKey(engine::Key::D, true);
  engine::Input::setScrollDelta(0.0f, 1.0f);
  engine::Scene scene;
  engine::SceneObject cam = scene.createObject("Cam");
  cam.getComponent<engine::TransformComponent>().translation = {0, 20, 0};
  engine::CameraComponent& cc = cam.addComponent<engine::CameraComponent>();
  cc.type = engine::ProjectionType::Orthographic;
  cc.orthoSize = 20.0f;
  engine::Camera2DControllerComponent& c2 =
      cam.addComponent<engine::Camera2DControllerComponent>();
  scene.update(1.0f);
  const engine::TransformComponent& t =
      cam.getComponent<engine::TransformComponent>();
  REQUIRE(t.translation.x ==
          Catch::Approx(c2.panSpeed).margin(1e-3));  // D -> +X
  REQUIRE(cam.getComponent<engine::CameraComponent>().orthoSize <
          20.0f);  // scroll up zooms in
  engine::Input::setKey(engine::Key::D, false);
  engine::Input::beginFrame();
}
