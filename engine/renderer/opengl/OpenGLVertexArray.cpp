#include "engine/renderer/opengl/OpenGLVertexArray.hpp"

#include <glad/glad.h>

#include "engine/renderer/opengl/OpenGLBuffer.hpp"

namespace engine {

static GLenum baseType(ShaderDataType type) {
  switch (type) {
    case ShaderDataType::Float:
    case ShaderDataType::Float2:
    case ShaderDataType::Float3:
    case ShaderDataType::Float4:
    case ShaderDataType::Mat3:
    case ShaderDataType::Mat4:
      return GL_FLOAT;
    case ShaderDataType::Int:
    case ShaderDataType::Int2:
    case ShaderDataType::Int3:
    case ShaderDataType::Int4:
      return GL_INT;
    case ShaderDataType::Bool:
      return GL_BOOL;
    case ShaderDataType::None:
      return GL_NONE;
  }
  return GL_NONE;
}

OpenGLVertexArray::OpenGLVertexArray() {
  glCreateVertexArrays(1, &m_rendererID);
}
OpenGLVertexArray::~OpenGLVertexArray() {
  glDeleteVertexArrays(1, &m_rendererID);
}

void OpenGLVertexArray::bind() const { glBindVertexArray(m_rendererID); }
void OpenGLVertexArray::unbind() const { glBindVertexArray(0); }

void OpenGLVertexArray::addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
  const uint32_t glId =
      static_cast<OpenGLVertexBuffer&>(*vertexBuffer).rendererID();
  const BufferLayout& layout = vertexBuffer->layout();
  const uint32_t binding = m_bindingIndex++;
  glVertexArrayVertexBuffer(m_rendererID, binding, glId, 0,
                            static_cast<GLsizei>(layout.stride()));
  for (const BufferElement& element : layout) {
    glEnableVertexArrayAttrib(m_rendererID, m_attribIndex);
    glVertexArrayAttribFormat(
        m_rendererID, m_attribIndex,
        static_cast<GLint>(element.componentCount()), baseType(element.type),
        element.normalized ? GL_TRUE : GL_FALSE, element.offset);
    glVertexArrayAttribBinding(m_rendererID, m_attribIndex, binding);
    m_attribIndex++;
  }
  m_vertexBuffers.push_back(vertexBuffer);
}

void OpenGLVertexArray::setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
  const uint32_t glId =
      static_cast<OpenGLIndexBuffer&>(*indexBuffer).rendererID();
  glVertexArrayElementBuffer(m_rendererID, glId);
  m_indexBuffer = indexBuffer;
}

Ref<VertexArray> VertexArray::Create() {
  return CreateRef<OpenGLVertexArray>();
}

}  // namespace engine
