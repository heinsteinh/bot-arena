#include "engine/renderer/opengl/OpenGLFramebuffer.hpp"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <vector>

namespace engine {

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpec& spec)
    : m_spec(spec) {
  invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer() { destroy(); }

void OpenGLFramebuffer::destroy() {
  if (m_rendererID) {
    glDeleteFramebuffers(1, &m_rendererID);
    if (!m_colors.empty()) {
      glDeleteTextures(static_cast<GLsizei>(m_colors.size()), m_colors.data());
    }
    if (m_depth) glDeleteTextures(1, &m_depth);
    m_rendererID = 0;
    m_colors.clear();
    m_depth = 0;
  }
}

void OpenGLFramebuffer::invalidate() {
  destroy();

  const GLsizei w = static_cast<GLsizei>(m_spec.width);
  const GLsizei h = static_cast<GLsizei>(m_spec.height);

  glCreateFramebuffers(1, &m_rendererID);

  if (m_spec.depthOnly) {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_depth);
    glTextureStorage2D(m_depth, 1, GL_DEPTH_COMPONENT32F, w, h);
    glTextureParameteri(m_depth, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(m_depth, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(m_depth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTextureParameteri(m_depth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTextureParameterfv(m_depth, GL_TEXTURE_BORDER_COLOR, border);
    glTextureParameteri(m_depth, GL_TEXTURE_COMPARE_MODE,
                        GL_COMPARE_REF_TO_TEXTURE);
    glTextureParameteri(m_depth, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glNamedFramebufferTexture(m_rendererID, GL_DEPTH_ATTACHMENT, m_depth, 0);
    glNamedFramebufferDrawBuffer(m_rendererID, GL_NONE);
    glNamedFramebufferReadBuffer(m_rendererID, GL_NONE);
  } else {
    std::vector<FramebufferFormat> formats = m_spec.colorFormats;
    if (formats.empty()) {
      formats.push_back(m_spec.hdr ? FramebufferFormat::RGBA16F
                                   : FramebufferFormat::RGBA8);
    }
    m_colors.resize(formats.size());
    glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(m_colors.size()),
                     m_colors.data());
    std::vector<GLenum> drawBuffers;
    for (size_t i = 0; i < formats.size(); ++i) {
      const GLenum internal =
          formats[i] == FramebufferFormat::RGBA16F ? GL_RGBA16F : GL_RGBA8;
      glTextureStorage2D(m_colors[i], 1, internal, w, h);
      glTextureParameteri(m_colors[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTextureParameteri(m_colors[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTextureParameteri(m_colors[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTextureParameteri(m_colors[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      const GLenum attachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
      glNamedFramebufferTexture(m_rendererID, attachment, m_colors[i], 0);
      drawBuffers.push_back(attachment);
    }
    glNamedFramebufferDrawBuffers(m_rendererID,
                                  static_cast<GLsizei>(drawBuffers.size()),
                                  drawBuffers.data());

    glCreateTextures(GL_TEXTURE_2D, 1, &m_depth);
    glTextureStorage2D(m_depth, 1, GL_DEPTH24_STENCIL8, w, h);
    glNamedFramebufferTexture(m_rendererID, GL_DEPTH_STENCIL_ATTACHMENT,
                              m_depth, 0);
  }

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
