#ifndef ENGINE_RENDERER_PERSPECTIVECAMERA_HPP
#define ENGINE_RENDERER_PERSPECTIVECAMERA_HPP

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.hpp"

namespace engine {

class PerspectiveCamera final : public Camera {
 public:
  void setPerspective(float fovDegrees, float aspectRatio, float nearPlane,
                      float farPlane) {
    m_fovDegrees = fovDegrees;
    m_aspectRatio = aspectRatio;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
  }

  void setPosition(const glm::vec3& position) { m_position = position; }

  void setRotation(float yawDegrees, float pitchDegrees) {
    m_yawDegrees = yawDegrees;
    m_pitchDegrees = pitchDegrees;
  }

  void lookAt(const glm::vec3& position, const glm::vec3& target) {
    m_position = position;

    const glm::vec3 direction = glm::normalize(target - position);

    m_pitchDegrees = glm::degrees(std::asin(direction.y));
    m_yawDegrees = glm::degrees(std::atan2(direction.z, direction.x));
  }

  glm::vec3 position() const { return m_position; }

  glm::vec3 forward() const {
    const float yaw = glm::radians(m_yawDegrees);
    const float pitch = glm::radians(m_pitchDegrees);

    return glm::normalize(glm::vec3{std::cos(yaw) * std::cos(pitch),
                                    std::sin(pitch),
                                    std::sin(yaw) * std::cos(pitch)});
  }

  glm::vec3 right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3{0.0f, 1.0f, 0.0f}));
  }

  glm::vec3 up() const {
    return glm::normalize(glm::cross(right(), forward()));
  }

  glm::mat4 view() const override {
    return glm::lookAt(m_position, m_position + forward(),
                       glm::vec3{0.0f, 1.0f, 0.0f});
  }

  glm::mat4 projection() const override {
    return glm::perspective(glm::radians(m_fovDegrees), m_aspectRatio,
                            m_nearPlane, m_farPlane);
  }

 private:
  glm::vec3 m_position{0.0f, 2.0f, 5.0f};

  float m_yawDegrees = -90.0f;
  float m_pitchDegrees = 0.0f;

  float m_fovDegrees = 60.0f;
  float m_aspectRatio = 16.0f / 9.0f;
  float m_nearPlane = 0.1f;
  float m_farPlane = 100.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_PERSPECTIVECAMERA_HPP
