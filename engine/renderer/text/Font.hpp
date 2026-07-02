#ifndef ENGINE_RENDERER_TEXT_FONT_HPP
#define ENGINE_RENDERER_TEXT_FONT_HPP

#include <cstdint>
#include <string>
#include <utility>

#include "engine/core/Base.hpp"
#include "engine/renderer/Texture2D.hpp"
#include "engine/renderer/text/TextLayout.hpp"

namespace engine {

// A baked bitmap font: an R8 atlas texture + ASCII glyph metrics.
class Font {
 public:
  Font(GlyphMap glyphs, Ref<Texture2D> atlas)
      : m_glyphs(glyphs), m_atlas(std::move(atlas)) {}

  static Ref<Font> Load(const std::string& ttfPath, uint32_t pixelSize);

  const GlyphMap& glyphs() const { return m_glyphs; }
  uint32_t atlasRendererID() const { return m_atlas->rendererID(); }
  void bindAtlas(uint32_t unit) const { m_atlas->bind(unit); }

 private:
  GlyphMap m_glyphs;
  Ref<Texture2D> m_atlas;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_FONT_HPP
