#ifndef ENGINE_RENDERER_TEXT_GLYPHATLAS_HPP
#define ENGINE_RENDERER_TEXT_GLYPHATLAS_HPP

#include <cstdint>

#include "engine/core/Base.hpp"
#include "engine/renderer/Texture2D.hpp"

namespace engine {

// Owns an atlas texture + its dimensions. Creation of the underlying texture is
// done by the caller (FontManager's atlas factory) so this header stays free of
// GL and is usable in headless tests with a stub Texture2D.
class GlyphAtlas {
 public:
  GlyphAtlas(Ref<Texture2D> texture, int width, int height)
      : m_texture(std::move(texture)), m_width(width), m_height(height) {}

  uint32_t rendererID() const { return m_texture->rendererID(); }
  void bind(uint32_t unit) const { m_texture->bind(unit); }
  int width() const { return m_width; }
  int height() const { return m_height; }

 private:
  Ref<Texture2D> m_texture;
  int m_width;
  int m_height;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_GLYPHATLAS_HPP
