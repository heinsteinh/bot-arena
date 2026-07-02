#ifndef ENGINE_RENDERER_TEXTURECUBE_HPP
#define ENGINE_RENDERER_TEXTURECUBE_HPP

#include <cstdint>

#include "engine/core/Base.hpp"

namespace engine {

class TextureCube {
 public:
  virtual ~TextureCube() = default;
  virtual void bind(uint32_t unit) const = 0;
  virtual uint32_t rendererID() const = 0;

  static Ref<TextureCube> Create(uint32_t size, uint32_t mipLevels = 1);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXTURECUBE_HPP
