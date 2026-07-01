#ifndef ENGINE_RENDERER_OPENGL_OPENGLTEXTURECUBE_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLTEXTURECUBE_HPP

#include "engine/renderer/TextureCube.hpp"

namespace engine {

class OpenGLTextureCube final : public TextureCube {
 public:
  OpenGLTextureCube(uint32_t size, uint32_t mipLevels);
  ~OpenGLTextureCube() override;

  void bind(uint32_t unit) const override;
  uint32_t rendererID() const override { return m_rendererID; }

 private:
  uint32_t m_rendererID = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLTEXTURECUBE_HPP
