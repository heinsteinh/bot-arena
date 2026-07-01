#ifndef ENGINE_RENDERER_CAMERA_HPP
#define ENGINE_RENDERER_CAMERA_HPP

#include <glm/glm.hpp>

namespace engine {

class Camera {
 public:
  virtual ~Camera() = default;

  virtual glm::mat4 view() const = 0;
  virtual glm::mat4 projection() const = 0;

  glm::mat4 viewProjection() const { return projection() * view(); }
};

}  // namespace engine

#endif  // ENGINE_RENDERER_CAMERA_HPP
