#ifndef ENGINE_RENDERER_TEXTURE2D_HPP
#define ENGINE_RENDERER_TEXTURE2D_HPP

#include <cstdint>

#include "engine/core/Base.hpp"

namespace engine {

// Single-channel (GL_R8) 2D texture with CPU upload -- used for the font atlas.
class Texture2D {
 public:
  virtual ~Texture2D() = default;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void bind(uint32_t unit) const = 0;
  virtual uint32_t rendererID() const = 0;
  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;

  static Ref<Texture2D> Create(uint32_t width, uint32_t height);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXTURE2D_HPP
