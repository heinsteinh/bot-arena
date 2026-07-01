#ifndef ENGINE_RENDERER_DEBUGRENDERER_HPP
#define ENGINE_RENDERER_DEBUGRENDERER_HPP

#include <glm/glm.hpp>

#include "engine/renderer/RenderQueue.hpp"

namespace engine {

class DebugRenderer {
 public:
  explicit DebugRenderer(RenderQueue& queue) : m_queue(queue) {}

  void drawLine(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color);
  void drawCube(const glm::vec3& center, const glm::vec3& size,
                const glm::vec4& color);
  void drawGrid(float halfSize, float spacing, const glm::vec4& color);

 private:
  RenderQueue& m_queue;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_DEBUGRENDERER_HPP
