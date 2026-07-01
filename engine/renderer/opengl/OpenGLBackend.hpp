#ifndef ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP

#include "engine/renderer/RenderBackend.hpp"

namespace engine {

class OpenGLBackend final : public RenderBackend {
 public:
  OpenGLBackend();
  ~OpenGLBackend() override;

  void beginFrame(int width, int height) override;
  void execute(const std::vector<RenderEntry>& entries,
               const glm::mat4& viewProjection, Arena& scratch) override;
  void endFrame() override;
  void readPixels(int x, int y, int width, int height, void* out) override;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_shader = 0;
  int m_vboCapacityBytes = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP
