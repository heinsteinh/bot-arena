#include "engine/renderer/opengl/OpenGLTextureCube.hpp"

#include <glad/glad.h>

namespace engine {

OpenGLTextureCube::OpenGLTextureCube(uint32_t size, uint32_t mipLevels) {
  glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_rendererID);
  glTextureStorage2D(m_rendererID, static_cast<GLsizei>(mipLevels), GL_RGBA16F,
                     static_cast<GLsizei>(size), static_cast<GLsizei>(size));
  glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER,
                      mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
  glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

OpenGLTextureCube::~OpenGLTextureCube() {
  if (m_rendererID) glDeleteTextures(1, &m_rendererID);
}

void OpenGLTextureCube::bind(uint32_t unit) const {
  glBindTextureUnit(unit, m_rendererID);
}

Ref<TextureCube> TextureCube::Create(uint32_t size, uint32_t mipLevels) {
  return CreateRef<OpenGLTextureCube>(size, mipLevels);
}

}  // namespace engine
