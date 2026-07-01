#ifndef ENGINE_RENDERER_CAMERA2D_HPP
#define ENGINE_RENDERER_CAMERA2D_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/renderer/Camera.hpp"

namespace engine {

class Camera2D final : public Camera {
 public:
  void setViewport(float width, float height) {
    m_width = width;
    m_height = height;
  }

  void setPosition(const glm::vec2& position) { m_position = position; }

  glm::mat4 view() const override {
    return glm::translate(glm::mat4{1.0f},
                          glm::vec3{-m_position.x, -m_position.y, 0.0f});
  }

  glm::mat4 projection() const override {
    return glm::ortho(0.0f, m_width, m_height, 0.0f, -1.0f, 1.0f);
  }

 private:
  glm::vec2 m_position{0.0f, 0.0f};

  float m_width = 1280.0f;
  float m_height = 720.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_CAMERA2D_HPP
