#ifndef ENGINE_RENDERER_OPENGL_OPENGLVERTEXARRAY_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLVERTEXARRAY_HPP

#include <vector>

#include "engine/renderer/VertexArray.hpp"

namespace engine {

class OpenGLVertexArray final : public VertexArray {
 public:
  OpenGLVertexArray();
  ~OpenGLVertexArray() override;

  void bind() const override;
  void unbind() const override;
  void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
  void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;
  const Ref<IndexBuffer>& indexBuffer() const override { return m_indexBuffer; }

 private:
  uint32_t m_rendererID = 0;
  uint32_t m_attribIndex = 0;
  uint32_t m_bindingIndex = 0;
  std::vector<Ref<VertexBuffer>> m_vertexBuffers;
  Ref<IndexBuffer> m_indexBuffer;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLVERTEXARRAY_HPP
