#ifndef ENGINE_SCENE_CONTROLLERCOMPONENTS_HPP
#define ENGINE_SCENE_CONTROLLERCOMPONENTS_HPP

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace engine {

struct FlyControllerComponent {
  float moveSpeed = 6.0f;
  float lookSensitivity = 0.08f;
  float yaw = -135.0f;
  float pitch = -30.0f;
};

struct OrbitControllerComponent {
  entt::entity target =
      entt::null;  // if valid, orbit its translation; else targetPoint
  glm::vec3 targetPoint{0.0f, 0.5f, 0.0f};
  float yaw = 45.0f;
  float pitch = 30.0f;
  float distance = 14.0f;
  float minDistance = 3.0f;
  float maxDistance = 40.0f;
  float rotateSpeed = 0.25f;
  float zoomSpeed = 1.5f;
};

struct FollowControllerComponent {
  entt::entity target = entt::null;  // required; camera sits at target +
                                     // offset, looking at target
  glm::vec3 offset{0.0f, 4.0f, 9.0f};
  float damping = 0.0f;  // 0 = hard follow; >0 = exponential smoothing
};

struct Camera2DControllerComponent {  // top-down ortho: looks down -Y, pans
                                      // X/Z, zooms orthoSize
  float panSpeed = 8.0f;
  float zoomSpeed = 1.5f;
};

}  // namespace engine

#endif  // ENGINE_SCENE_CONTROLLERCOMPONENTS_HPP
