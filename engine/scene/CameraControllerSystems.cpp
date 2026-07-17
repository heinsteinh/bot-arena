#include "engine/scene/CameraControllerSystems.hpp"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "engine/core/Input.hpp"
#include "engine/scene/CameraMath.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/ControllerComponents.hpp"

namespace engine {

void updateFlyControllers(entt::registry& registry, float dt) {
  auto view = registry.view<TransformComponent, FlyControllerComponent>();
  for (const entt::entity e : view) {
    TransformComponent& t = view.get<TransformComponent>(e);
    FlyControllerComponent& fc = view.get<FlyControllerComponent>(e);
    if (Input::isMouseButtonDown(MouseButton::Right)) {
      const glm::vec2 md = Input::mouseDelta();
      fc.yaw += md.x * fc.lookSensitivity;
      fc.pitch -= md.y * fc.lookSensitivity;
      fc.pitch = std::clamp(fc.pitch, -89.0f, 89.0f);
    }
    const glm::quat orient = orientationFromYawPitch(fc.yaw, fc.pitch);
    const glm::vec3 f = forwardDir(orient);
    const glm::vec3 planar = glm::normalize(glm::vec3(f.x, 0.0f, f.z));
    const glm::vec3 right = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
    glm::vec3 move(0.0f);
    if (Input::isKeyDown(Key::W)) move += planar;
    if (Input::isKeyDown(Key::S)) move -= planar;
    if (Input::isKeyDown(Key::D)) move += right;
    if (Input::isKeyDown(Key::A)) move -= right;
    if (Input::isKeyDown(Key::E)) move.y += 1.0f;
    if (Input::isKeyDown(Key::Q)) move.y -= 1.0f;
    if (glm::dot(move, move) > 0.0f)
      t.translation += glm::normalize(move) * fc.moveSpeed * dt;
    t.rotation = orient;
  }
}

void updateOrbitControllers(entt::registry& registry, float dt) {
  (void)dt;
  auto view = registry.view<TransformComponent, OrbitControllerComponent>();
  for (const entt::entity e : view) {
    TransformComponent& t = view.get<TransformComponent>(e);
    OrbitControllerComponent& oc = view.get<OrbitControllerComponent>(e);
    if (Input::isMouseButtonDown(MouseButton::Left)) {
      const glm::vec2 md = Input::mouseDelta();
      oc.yaw += md.x * oc.rotateSpeed;
      oc.pitch -= md.y * oc.rotateSpeed;
      oc.pitch = std::clamp(oc.pitch, -89.0f, 89.0f);
    }
    oc.distance -= Input::scrollDelta().y * oc.zoomSpeed;
    oc.distance = std::clamp(oc.distance, oc.minDistance, oc.maxDistance);
    glm::vec3 center = oc.targetPoint;
    if (registry.valid(oc.target) &&
        registry.all_of<TransformComponent>(oc.target))
      center = registry.get<TransformComponent>(oc.target).translation;
    t.translation = orbitPosition(center, oc.yaw, oc.pitch, oc.distance);
    t.rotation = lookRotation(center - t.translation);
  }
}

void updateFollowControllers(entt::registry& registry, float dt) {
  auto view = registry.view<TransformComponent, FollowControllerComponent>();
  for (const entt::entity e : view) {
    TransformComponent& t = view.get<TransformComponent>(e);
    FollowControllerComponent& fc = view.get<FollowControllerComponent>(e);
    if (!registry.valid(fc.target) ||
        !registry.all_of<TransformComponent>(fc.target))
      continue;
    const glm::vec3 targetPos =
        registry.get<TransformComponent>(fc.target).translation;
    const glm::vec3 goal = targetPos + fc.offset;
    if (fc.damping > 0.0f) {
      const float a = 1.0f - std::exp(-fc.damping * dt);
      t.translation = glm::mix(t.translation, goal, a);
    } else {
      t.translation = goal;
    }
    t.rotation = lookRotation(targetPos - t.translation);
  }
}

void updateCamera2DControllers(entt::registry& registry, float dt) {
  auto view = registry.view<TransformComponent, CameraComponent,
                            Camera2DControllerComponent>();
  for (const entt::entity e : view) {
    TransformComponent& t = view.get<TransformComponent>(e);
    CameraComponent& cam = view.get<CameraComponent>(e);
    Camera2DControllerComponent& cc = view.get<Camera2DControllerComponent>(e);
    glm::vec3 pan(0.0f);
    if (Input::isKeyDown(Key::D)) pan.x += 1.0f;
    if (Input::isKeyDown(Key::A)) pan.x -= 1.0f;
    if (Input::isKeyDown(Key::W)) pan.z -= 1.0f;
    if (Input::isKeyDown(Key::S)) pan.z += 1.0f;
    t.translation += pan * cc.panSpeed * dt;
    cam.orthoSize -= Input::scrollDelta().y * cc.zoomSpeed;
    cam.orthoSize = std::max(cam.orthoSize, 0.5f);
    t.rotation = lookRotation(glm::vec3(0.0f, -1.0f, 0.0f),
                              glm::vec3(0.0f, 0.0f, -1.0f));  // top-down
  }
}

}  // namespace engine
