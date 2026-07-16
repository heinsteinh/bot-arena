#ifndef ENGINE_RENDERER_MATRIXCAMERA_HPP
#define ENGINE_RENDERER_MATRIXCAMERA_HPP

#include <glm/glm.hpp>

#include "engine/renderer/Camera.hpp"

namespace engine {

// A Camera backed by precomputed matrices (e.g. from a Scene's CameraUniforms),
// so existing consumers that take a const Camera& (like MeshRenderer) can use a
// scene-driven camera without reconstructing one.
class MatrixCamera final : public Camera {
 public:
  MatrixCamera(const glm::mat4& view, const glm::mat4& projection)
      : m_view(view), m_projection(projection) {}
  glm::mat4 view() const override { return m_view; }
  glm::mat4 projection() const override { return m_projection; }

 private:
  glm::mat4 m_view;
  glm::mat4 m_projection;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_MATRIXCAMERA_HPP
