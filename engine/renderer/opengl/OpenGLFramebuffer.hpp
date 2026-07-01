#ifndef ENGINE_RENDERER_OPENGL_OPENGLFRAMEBUFFER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLFRAMEBUFFER_HPP

#include <vector>

#include "engine/renderer/Framebuffer.hpp"

namespace engine {

class OpenGLFramebuffer final : public Framebuffer {
 public:
  explicit OpenGLFramebuffer(const FramebufferSpec& spec);
  ~OpenGLFramebuffer() override;

  void bind() override;
  void unbind() override;
  void resize(uint32_t width, uint32_t height) override;
  uint32_t colorAttachment(uint32_t index = 0) const override {
    return m_colors.empty() ? 0 : m_colors[index];
  }
  uint32_t depthAttachment() const override { return m_depth; }

 private:
  void invalidate();
  void destroy();

  FramebufferSpec m_spec;
  uint32_t m_rendererID = 0;
  std::vector<uint32_t> m_colors;
  uint32_t m_depth = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLFRAMEBUFFER_HPP
