#ifndef ENGINE_RENDERER_OPENGL_OPENGLTEXTURE2D_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLTEXTURE2D_HPP

#include "engine/renderer/Texture2D.hpp"

namespace engine {

class OpenGLTexture2D final : public Texture2D {
 public:
  OpenGLTexture2D(uint32_t width, uint32_t height, TextureFormat format);
  ~OpenGLTexture2D() override;

  void setData(const void* data, uint32_t size) override;
  void bind(uint32_t unit) const override;
  uint32_t rendererID() const override { return m_rendererID; }
  uint32_t width() const override { return m_width; }
  uint32_t height() const override { return m_height; }

 private:
  uint32_t m_rendererID = 0;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  TextureFormat m_format = TextureFormat::R8;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLTEXTURE2D_HPP
