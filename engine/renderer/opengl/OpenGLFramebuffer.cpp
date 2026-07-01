#include "engine/renderer/opengl/OpenGLFramebuffer.hpp"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace engine {

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpec& spec)
    : m_spec(spec) {
  invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer() { destroy(); }

void OpenGLFramebuffer::destroy() {
  if (m_rendererID) {
    glDeleteFramebuffers(1, &m_rendererID);
    glDeleteTextures(1, &m_color);
    glDeleteTextures(1, &m_depth);
    m_rendererID = 0;
    m_color = 0;
    m_depth = 0;
  }
}

void OpenGLFramebuffer::invalidate() {
  destroy();

  const GLsizei w = static_cast<GLsizei>(m_spec.width);
  const GLsizei h = static_cast<GLsizei>(m_spec.height);

  glCreateFramebuffers(1, &m_rendererID);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_color);
  glTextureStorage2D(m_color, 1, m_spec.hdr ? GL_RGBA16F : GL_RGBA8, w, h);
  glTextureParameteri(m_color, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTextureParameteri(m_color, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(m_color, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(m_color, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glNamedFramebufferTexture(m_rendererID, GL_COLOR_ATTACHMENT0, m_color, 0);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_depth);
  glTextureStorage2D(m_depth, 1, GL_DEPTH24_STENCIL8, w, h);
  glNamedFramebufferTexture(m_rendererID, GL_DEPTH_STENCIL_ATTACHMENT, m_depth,
                            0);

  if (glCheckNamedFramebufferStatus(m_rendererID, GL_FRAMEBUFFER) !=
      GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Framebuffer incomplete");
  }
}

void OpenGLFramebuffer::bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
  glViewport(0, 0, static_cast<GLsizei>(m_spec.width),
             static_cast<GLsizei>(m_spec.height));
}

void OpenGLFramebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void OpenGLFramebuffer::resize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) return;
  if (width == m_spec.width && height == m_spec.height) return;
  m_spec.width = width;
  m_spec.height = height;
  invalidate();
}

Ref<Framebuffer> Framebuffer::Create(const FramebufferSpec& spec) {
  return CreateRef<OpenGLFramebuffer>(spec);
}

}  // namespace engine
