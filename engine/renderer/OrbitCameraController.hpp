#ifndef ENGINE_RENDERER_ORBITCAMERACONTROLLER_HPP
#define ENGINE_RENDERER_ORBITCAMERACONTROLLER_HPP

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/core/Input.hpp"
#include "engine/renderer/CameraController.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"

namespace engine {

// Orbits a target point: left-drag rotates around it, scroll wheel zooms
// (dollies the camera toward/away from the target).
class OrbitCameraController final : public CameraController {
 public:
  void setTarget(const glm::vec3& target) { m_target = target; }

  void setOrbit(float yawDegrees, float pitchDegrees, float distance) {
    m_yaw = yawDegrees;
    m_pitch = pitchDegrees;
    m_distance = distance;
  }

  void update(float /*dt*/) override {
    const glm::vec2 mouseDelta = Input::mouseDelta();

    if (Input::isMouseButtonDown(MouseButton::Left)) {
      m_yaw += mouseDelta.x * m_rotateSpeed;
      m_pitch -= mouseDelta.y * m_rotateSpeed;
      m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    m_distance -= Input::scrollDelta().y * m_zoomSpeed;
    m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance);

    const float yaw = glm::radians(m_yaw);
    const float pitch = glm::radians(m_pitch);

    const glm::vec3 offset{std::cos(pitch) * std::cos(yaw), std::sin(pitch),
                           std::cos(pitch) * std::sin(yaw)};

    m_camera.lookAt(m_target + offset * m_distance, m_target);
  }

  void resize(float aspectRatio) override {
    m_camera.setPerspective(m_fovDegrees, aspectRatio, m_nearPlane, m_farPlane);
  }

  Camera& camera() override { return m_camera; }
  const Camera& camera() const override { return m_camera; }

 private:
  PerspectiveCamera m_camera;

  glm::vec3 m_target{0.0f, 0.5f, 0.0f};

  float m_yaw = 45.0f;
  float m_pitch = 30.0f;
  float m_distance = 14.0f;

  float m_minDistance = 3.0f;
  float m_maxDistance = 40.0f;

  float m_rotateSpeed = 0.25f;
  float m_zoomSpeed = 1.5f;

  float m_fovDegrees = 60.0f;
  float m_nearPlane = 0.1f;
  float m_farPlane = 100.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_ORBITCAMERACONTROLLER_HPP
