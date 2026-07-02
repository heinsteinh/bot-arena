#include "engine/renderer/opengl/OpenGLTexture2D.hpp"

#include <glad/glad.h>

namespace engine {

OpenGLTexture2D::OpenGLTexture2D(uint32_t width, uint32_t height,
                                 TextureFormat format)
    : m_width(width), m_height(height), m_format(format) {
  const GLenum internal = format == TextureFormat::RGBA8 ? GL_RGBA8 : GL_R8;
  glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
  glTextureStorage2D(m_rendererID, 1, internal, static_cast<GLsizei>(width),
                     static_cast<GLsizei>(height));
  glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

OpenGLTexture2D::~OpenGLTexture2D() {
  if (m_rendererID) glDeleteTextures(1, &m_rendererID);
}

void OpenGLTexture2D::setData(const void* data, uint32_t /*size*/) {
  const GLenum fmt = m_format == TextureFormat::RGBA8 ? GL_RGBA : GL_RED;
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTextureSubImage2D(m_rendererID, 0, 0, 0, static_cast<GLsizei>(m_width),
                      static_cast<GLsizei>(m_height), fmt, GL_UNSIGNED_BYTE,
                      data);
}

void OpenGLTexture2D::bind(uint32_t unit) const {
  glBindTextureUnit(unit, m_rendererID);
}

Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height,
                                 TextureFormat format) {
  return CreateRef<OpenGLTexture2D>(width, height, format);
}

}  // namespace engine
