#ifndef ENGINE_RENDERER_VERTEXARRAY_HPP
#define ENGINE_RENDERER_VERTEXARRAY_HPP

#include "engine/core/Base.hpp"
#include "engine/renderer/Buffer.hpp"

namespace engine {

class VertexArray {
 public:
  virtual ~VertexArray() = default;
  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
  virtual void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;
  virtual const Ref<IndexBuffer>& indexBuffer() const = 0;

  static Ref<VertexArray> Create();
};

}  // namespace engine

#endif  // ENGINE_RENDERER_VERTEXARRAY_HPP
