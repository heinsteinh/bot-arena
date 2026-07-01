#include "engine/renderer/DebugRenderer.hpp"

namespace engine {

void DebugRenderer::drawLine(const glm::vec3& a, const glm::vec3& b,
                             const glm::vec4& color) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::Line;
  cmd.layer = RenderLayer::Debug;
  cmd.lineStart = a;
  cmd.lineEnd = b;
  cmd.color = color;
  m_queue.submit(cmd);
}

void DebugRenderer::drawCube(const glm::vec3& center, const glm::vec3& size,
                             const glm::vec4& color) {
  RenderCommand cmd;
  cmd.type = RenderCommandType::Cube;
  cmd.layer = RenderLayer::Opaque;
  cmd.position = center;
  cmd.scale = size;
  cmd.color = color;
  m_queue.submit(cmd);
}

void DebugRenderer::drawGrid(float halfSize, float spacing,
                             const glm::vec4& color) {
  for (float i = -halfSize; i <= halfSize; i += spacing) {
    RenderCommand a;
    a.type = RenderCommandType::Line;
    a.layer = RenderLayer::Grid;
    a.lineStart = {i, 0.0f, -halfSize};
    a.lineEnd = {i, 0.0f, halfSize};
    a.color = color;
    m_queue.submit(a);

    RenderCommand b;
    b.type = RenderCommandType::Line;
    b.layer = RenderLayer::Grid;
    b.lineStart = {-halfSize, 0.0f, i};
    b.lineEnd = {halfSize, 0.0f, i};
    b.color = color;
    m_queue.submit(b);
  }
}

}  // namespace engine
