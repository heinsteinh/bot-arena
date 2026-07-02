#ifndef ENGINE_RENDERER_TEXT_TEXTLAYOUT_HPP
#define ENGINE_RENDERER_TEXT_TEXTLAYOUT_HPP

#include <array>
#include <glm/glm.hpp>
#include <string_view>
#include <vector>

namespace engine {

struct Glyph {
  glm::vec2 size{0.0f};     // px (width, height)
  glm::vec2 bearing{0.0f};  // px (left, top-above-baseline)
  float advance = 0.0f;     // px
  glm::vec2 uvMin{0.0f};
  glm::vec2 uvMax{0.0f};
};

using GlyphMap = std::array<Glyph, 128>;  // ASCII-indexed

struct TextQuad {
  float x0, y0, x1, y1;  // pixel-space corners (top-left, bottom-right)
  float u0, v0, u1, v1;  // atlas UVs
};

// Lay out `text` at baseline pen (x, y), pixel space, y-down. Zero-size glyphs
// advance without a quad; chars >= 128 or absent from the map are skipped.
inline std::vector<TextQuad> layoutText(const GlyphMap& glyphs,
                                        std::string_view text, float x, float y,
                                        float scale) {
  std::vector<TextQuad> quads;
  float penX = x;
  for (char ch : text) {
    const unsigned char uc = static_cast<unsigned char>(ch);
    if (uc >= 128) continue;
    const Glyph& g = glyphs[uc];
    if (g.size.x > 0.0f && g.size.y > 0.0f) {
      TextQuad q;
      q.x0 = penX + g.bearing.x * scale;
      q.y0 = y - g.bearing.y * scale;
      q.x1 = q.x0 + g.size.x * scale;
      q.y1 = q.y0 + g.size.y * scale;
      q.u0 = g.uvMin.x;
      q.v0 = g.uvMin.y;
      q.u1 = g.uvMax.x;
      q.v1 = g.uvMax.y;
      quads.push_back(q);
    }
    penX += g.advance * scale;
  }
  return quads;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTLAYOUT_HPP
