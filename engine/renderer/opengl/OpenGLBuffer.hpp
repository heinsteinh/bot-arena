#ifndef ENGINE_RENDERER_OPENGL_OPENGLBUFFER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLBUFFER_HPP

#include "engine/renderer/Buffer.hpp"

namespace engine {

class OpenGLVertexBuffer final : public VertexBuffer {
 public:
  explicit OpenGLVertexBuffer(uint32_t size);
  OpenGLVertexBuffer(const void* data, uint32_t size);
  ~OpenGLVertexBuffer() override;

  void bind() const override;
  void unbind() const override;
  void setData(const void* data, uint32_t size) override;
  void setLayout(const BufferLayout& layout) override { m_layout = layout; }
  const BufferLayout& layout() const override { return m_layout; }

  uint32_t rendererID() const { return m_rendererID; }

 private:
  uint32_t m_rendererID = 0;
  BufferLayout m_layout;
};

class OpenGLIndexBuffer final : public IndexBuffer {
 public:
  OpenGLIndexBuffer(const uint32_t* indices, uint32_t count);
  ~OpenGLIndexBuffer() override;

  void bind() const override;
  void unbind() const override;
  uint32_t count() const override { return m_count; }

  uint32_t rendererID() const { return m_rendererID; }

 private:
  uint32_t m_rendererID = 0;
  uint32_t m_count = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLBUFFER_HPP
