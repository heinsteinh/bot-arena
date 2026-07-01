#ifndef ENGINE_RENDERER_ORTHOGRAPHICCAMERA_HPP
#define ENGINE_RENDERER_ORTHOGRAPHICCAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.hpp"

namespace engine {

class OrthographicCamera final : public Camera {
 public:
  void setBounds(float left, float right, float bottom, float top,
                 float nearPlane, float farPlane) {
    m_left = left;
    m_right = right;
    m_bottom = bottom;
    m_top = top;
    m_nearPlane = nearPlane;
    m_farPlane = farPlane;
  }

  void lookAt(const glm::vec3& position, const glm::vec3& target) {
    m_position = position;
    m_target = target;
  }

  glm::mat4 view() const override {
    return glm::lookAt(m_position, m_target, glm::vec3{0.0f, 1.0f, 0.0f});
  }

  glm::mat4 projection() const override {
    return glm::ortho(m_left, m_right, m_bottom, m_top, m_nearPlane,
                      m_farPlane);
  }

 private:
  glm::vec3 m_position{0.0f, 10.0f, 0.0f};
  glm::vec3 m_target{0.0f, 0.0f, 0.0f};

  float m_left = -10.0f;
  float m_right = 10.0f;
  float m_bottom = -10.0f;
  float m_top = 10.0f;
  float m_nearPlane = -100.0f;
  float m_farPlane = 100.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_ORTHOGRAPHICCAMERA_HPP
