#include "engine/renderer/opengl/OpenGLBuffer.hpp"

#include <glad/glad.h>

namespace engine {

Ref<VertexBuffer> VertexBuffer::Create(uint32_t size) {
  return CreateRef<OpenGLVertexBuffer>(size);
}

Ref<VertexBuffer> VertexBuffer::Create(const void* data, uint32_t size) {
  return CreateRef<OpenGLVertexBuffer>(data, size);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, uint32_t size) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, size, data, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_rendererID); }

void OpenGLVertexBuffer::bind() const {
  glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
}
void OpenGLVertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void OpenGLVertexBuffer::setData(const void* data, uint32_t size) {
  glNamedBufferSubData(m_rendererID, 0, size, data);
}

Ref<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count) {
  return CreateRef<OpenGLIndexBuffer>(indices, count);
}

OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32_t* indices, uint32_t count)
    : m_count(count) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, count * sizeof(uint32_t), indices,
                    GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_rendererID); }

void OpenGLIndexBuffer::bind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
}
void OpenGLIndexBuffer::unbind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace engine
