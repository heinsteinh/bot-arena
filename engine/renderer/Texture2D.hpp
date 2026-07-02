#ifndef ENGINE_RENDERER_TEXTURE2D_HPP
#define ENGINE_RENDERER_TEXTURE2D_HPP

#include <cstdint>

#include "engine/core/Base.hpp"

namespace engine {

enum class TextureFormat { R8, RGBA8 };

// 2D texture with CPU upload -- R8 for the font atlas, RGBA8 for image
// textures.
class Texture2D {
 public:
  virtual ~Texture2D() = default;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void bind(uint32_t unit) const = 0;
  virtual uint32_t rendererID() const = 0;
  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;

  static Ref<Texture2D> Create(uint32_t width, uint32_t height,
                               TextureFormat format = TextureFormat::R8);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXTURE2D_HPP
