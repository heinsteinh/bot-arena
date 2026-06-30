#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

class Camera {
 public:
  Camera() = default;

  void setPerspective(float fovDegrees, float aspectRatio, float nearPlane,
                      float farPlane) {
    m_projection = glm::perspective(glm::radians(fovDegrees), aspectRatio,
                                    nearPlane, farPlane);
  }

  void lookAt(const glm::vec3& position, const glm::vec3& target,
              const glm::vec3& up = {0.0f, 1.0f, 0.0f}) {
    m_position = position;
    m_view = glm::lookAt(position, target, up);
  }

  const glm::mat4& view() const { return m_view; }

  const glm::mat4& projection() const { return m_projection; }

  glm::mat4 viewProjection() const { return m_projection * m_view; }

  const glm::vec3& position() const { return m_position; }

 private:
  glm::vec3 m_position{0.0f, 0.0f, 0.0f};

  glm::mat4 m_view{1.0f};
  glm::mat4 m_projection{1.0f};
};

}  // namespace engine