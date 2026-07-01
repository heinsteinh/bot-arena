#ifndef ENGINE_RENDERER_OPENGL_OPENGLUNIFORMBUFFER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLUNIFORMBUFFER_HPP

#include "engine/renderer/UniformBuffer.hpp"

namespace engine {

class OpenGLUniformBuffer final : public UniformBuffer {
 public:
  OpenGLUniformBuffer(uint32_t size, uint32_t binding);
  ~OpenGLUniformBuffer() override;

  void setData(const void* data, uint32_t size, uint32_t offset = 0) override;

 private:
  uint32_t m_rendererID = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLUNIFORMBUFFER_HPP
