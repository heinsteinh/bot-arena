#ifndef ENGINE_RENDERER_BUFFER_HPP
#define ENGINE_RENDERER_BUFFER_HPP

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "engine/core/Base.hpp"

namespace engine {

enum class ShaderDataType {
  None = 0,
  Float,
  Float2,
  Float3,
  Float4,
  Mat3,
  Mat4,
  Int,
  Int2,
  Int3,
  Int4,
  Bool
};

inline uint32_t ShaderDataTypeSize(ShaderDataType type) {
  switch (type) {
    case ShaderDataType::Float:
      return 4;
    case ShaderDataType::Float2:
      return 4 * 2;
    case ShaderDataType::Float3:
      return 4 * 3;
    case ShaderDataType::Float4:
      return 4 * 4;
    case ShaderDataType::Mat3:
      return 4 * 3 * 3;
    case ShaderDataType::Mat4:
      return 4 * 4 * 4;
    case ShaderDataType::Int:
      return 4;
    case ShaderDataType::Int2:
      return 4 * 2;
    case ShaderDataType::Int3:
      return 4 * 3;
    case ShaderDataType::Int4:
      return 4 * 4;
    case ShaderDataType::Bool:
      return 1;
    case ShaderDataType::None:
      return 0;
  }
  return 0;
}

struct BufferElement {
  std::string name;
  ShaderDataType type = ShaderDataType::None;
  uint32_t size = 0;
  uint32_t offset = 0;
  bool normalized = false;

  BufferElement() = default;
  BufferElement(ShaderDataType t, const std::string& n, bool norm = false)
      : name(n),
        type(t),
        size(ShaderDataTypeSize(t)),
        offset(0),
        normalized(norm) {}

  uint32_t componentCount() const {
    switch (type) {
      case ShaderDataType::Float:
        return 1;
      case ShaderDataType::Float2:
        return 2;
      case ShaderDataType::Float3:
        return 3;
      case ShaderDataType::Float4:
        return 4;
      case ShaderDataType::Mat3:
        return 3 * 3;
      case ShaderDataType::Mat4:
        return 4 * 4;
      case ShaderDataType::Int:
        return 1;
      case ShaderDataType::Int2:
        return 2;
      case ShaderDataType::Int3:
        return 3;
      case ShaderDataType::Int4:
        return 4;
      case ShaderDataType::Bool:
        return 1;
      case ShaderDataType::None:
        return 0;
    }
    return 0;
  }
};

class BufferLayout {
 public:
  BufferLayout() = default;
  BufferLayout(std::initializer_list<BufferElement> elements)
      : m_elements(elements) {
    uint32_t offset = 0;
    for (auto& e : m_elements) {
      e.offset = offset;
      offset += e.size;
      m_stride += e.size;
    }
  }

  uint32_t stride() const { return m_stride; }
  const std::vector<BufferElement>& elements() const { return m_elements; }
  std::vector<BufferElement>::const_iterator begin() const {
    return m_elements.begin();
  }
  std::vector<BufferElement>::const_iterator end() const {
    return m_elements.end();
  }

 private:
  std::vector<BufferElement> m_elements;
  uint32_t m_stride = 0;
};

class VertexBuffer {
 public:
  virtual ~VertexBuffer() = default;
  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void setLayout(const BufferLayout& layout) = 0;
  virtual const BufferLayout& layout() const = 0;

  static Ref<VertexBuffer> Create(uint32_t size);
  static Ref<VertexBuffer> Create(const void* data, uint32_t size);
};

class IndexBuffer {
 public:
  virtual ~IndexBuffer() = default;
  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual uint32_t count() const = 0;

  static Ref<IndexBuffer> Create(const uint32_t* indices, uint32_t count);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_BUFFER_HPP
