#ifndef ENGINE_RENDERER_FLYCAMERACONTROLLER_HPP
#define ENGINE_RENDERER_FLYCAMERACONTROLLER_HPP

#include <algorithm>
#include <glm/glm.hpp>

#include "engine/core/Input.hpp"
#include "engine/renderer/CameraController.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"

namespace engine {

// Free-fly camera: WASD/QE moves, right-drag looks around.
class FlyCameraController final : public CameraController {
 public:
  void setPose(const glm::vec3& position, float yawDegrees,
               float pitchDegrees) {
    m_position = position;
    m_yaw = yawDegrees;
    m_pitch = pitchDegrees;

    m_camera.setPosition(m_position);
    m_camera.setRotation(m_yaw, m_pitch);
  }

  void update(float dt) override {
    const glm::vec2 mouseDelta = Input::mouseDelta();

    if (Input::isMouseButtonDown(MouseButton::Right)) {
      m_yaw += mouseDelta.x * m_sensitivity;
      m_pitch -= mouseDelta.y * m_sensitivity;
      m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);
    }

    m_camera.setRotation(m_yaw, m_pitch);

    glm::vec3 forward = m_camera.forward();
    glm::vec3 right = m_camera.right();

    forward.y = 0.0f;
    right.y = 0.0f;

    forward = glm::normalize(forward);
    right = glm::normalize(right);

    if (Input::isKeyDown(Key::W)) {
      m_position += forward * m_moveSpeed * dt;
    }

    if (Input::isKeyDown(Key::S)) {
      m_position -= forward * m_moveSpeed * dt;
    }

    if (Input::isKeyDown(Key::D)) {
      m_position += right * m_moveSpeed * dt;
    }

    if (Input::isKeyDown(Key::A)) {
      m_position -= right * m_moveSpeed * dt;
    }

    if (Input::isKeyDown(Key::E)) {
      m_position.y += m_moveSpeed * dt;
    }

    if (Input::isKeyDown(Key::Q)) {
      m_position.y -= m_moveSpeed * dt;
    }

    m_camera.setPosition(m_position);
  }

  void resize(float aspectRatio) override {
    m_camera.setPerspective(m_fovDegrees, aspectRatio, m_nearPlane, m_farPlane);
  }

  Camera& camera() override { return m_camera; }
  const Camera& camera() const override { return m_camera; }

 private:
  PerspectiveCamera m_camera;

  glm::vec3 m_position{8.0f, 7.0f, 8.0f};
  float m_yaw = -135.0f;
  float m_pitch = -30.0f;

  float m_moveSpeed = 6.0f;
  float m_sensitivity = 0.08f;

  float m_fovDegrees = 60.0f;
  float m_nearPlane = 0.1f;
  float m_farPlane = 100.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_FLYCAMERACONTROLLER_HPP
