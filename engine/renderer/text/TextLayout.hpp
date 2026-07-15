#ifndef ENGINE_RENDERER_TEXT_TEXTLAYOUT_HPP
#define ENGINE_RENDERER_TEXT_TEXTLAYOUT_HPP

#include <functional>
#include <optional>
#include <string_view>
#include <vector>

#include "engine/renderer/text/Glyph.hpp"

namespace engine {

struct TextQuad {
  float x0, y0, x1, y1;  // local pixel corners (top-left, bottom-right)
  float u0, v0, u1, v1;  // atlas UVs
};

// Replacement lookup for a codepoint absent from `glyphs`. Return a Glyph to
// substitute, or nullptr to skip. Empty hook -> skip (current behavior).
using MissingGlyphFn = std::function<const Glyph*(char32_t)>;

struct TextLayoutState {
  float penX = 0.0f;
  float penY = 0.0f;  // reserved: multiline not yet supported
  std::optional<char32_t>
      previousGlyph;  // reserved: kerning seam (no kerning today)
};

// Append `text`'s quads to `out`, continuing from `state` and advancing it.
// Local pixel coords, y-down, baseline at penY. Missing/zero-size glyphs
// advance without a quad (identical to the current layout). Consecutive calls
// sharing one state lay out continuously -- span boundaries do not affect
// geometry.
inline void appendTextLayout(const GlyphStore& glyphs, std::string_view text,
                             TextLayoutState& state, std::vector<TextQuad>& out,
                             const MissingGlyphFn& onMissing = {}) {
  for (char ch : text) {
    const char32_t cp = static_cast<unsigned char>(ch);
    const Glyph* g = nullptr;
    if (auto it = glyphs.find(cp); it != glyphs.end()) {
      g = &it->second;
    } else if (onMissing) {
      g = onMissing(cp);
    }
    if (!g) continue;  // missing and no substitute: skip
    if (g->size.x > 0.0f && g->size.y > 0.0f) {
      TextQuad q;
      q.x0 = state.penX + g->bearing.x;
      q.y0 = state.penY - g->bearing.y;
      q.x1 = q.x0 + g->size.x;
      q.y1 = q.y0 + g->size.y;
      q.u0 = g->uvMin.x;
      q.v0 = g->uvMin.y;
      q.u1 = g->uvMax.x;
      q.v1 = g->uvMax.y;
      out.push_back(q);
    }
    state.penX += g->advance;
    state.previousGlyph = cp;
  }
}

// Lay out `text` in LOCAL coordinates: pen starts at (0,0), baseline at y=0,
// advancing +x, y-down. Surface-agnostic; the caller applies placement and
// projection. Zero-size glyphs advance without a quad. Bytes are treated as
// ASCII codepoints for now (>=128 -> missing-glyph hook).
inline std::vector<TextQuad> layoutText(const GlyphStore& glyphs,
                                        std::string_view text,
                                        const MissingGlyphFn& onMissing = {}) {
  TextLayoutState state;
  std::vector<TextQuad> out;
  appendTextLayout(glyphs, text, state, out, onMissing);
  return out;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTLAYOUT_HPP
