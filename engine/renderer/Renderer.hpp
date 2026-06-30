#ifndef ENGINE_RENDERER_RENDERER_HPP
#define ENGINE_RENDERER_RENDERER_HPP

#include <glm/glm.hpp>

#include "engine/renderer/Camera.hpp"

namespace engine {

namespace engine {

class Renderer {
 public:
  virtual ~Renderer() = default;

  virtual void beginFrame(int width, int height) = 0;
  virtual void endFrame() = 0;
  virtual void clear(const glm::vec4& color) = 0;

  virtual void setCamera(const Camera& camera) = 0;

  virtual void drawLine(const glm::vec3& a, const glm::vec3& b,
                        const glm::vec4& color) = 0;
  virtual void drawCube(const glm::vec3& center, const glm::vec3& size,
                        const glm::vec4& color) = 0;
  virtual void drawGrid(float halfSize, float spacing,
                        const glm::vec4& color) = 0;
};
}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERER_HPP
