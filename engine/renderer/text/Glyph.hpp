#ifndef ENGINE_RENDERER_TEXT_GLYPH_HPP
#define ENGINE_RENDERER_TEXT_GLYPH_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>

namespace engine {

// One rasterized glyph's metrics + atlas placement. Positions in pixels.
struct Glyph {
  glm::vec2 size{0.0f};     // px (width, height)
  glm::vec2 bearing{0.0f};  // px (left, top-above-baseline)
  float advance = 0.0f;     // px
  glm::vec2 uvMin{0.0f};
  glm::vec2 uvMax{0.0f};
  uint32_t glyphIndex = 0;  // reserved: face glyph index for future shaping
};

// Codepoint-keyed glyph metrics. Replaces the old ASCII array so Unicode,
// fallback, and lazy runtime population are additive.
using GlyphStore = std::unordered_map<char32_t, Glyph>;

// Face-wide vertical metrics (px). Stored now; used by multi-line layout later.
struct FaceMetrics {
  float ascent = 0.0f;
  float descent = 0.0f;
  float lineGap = 0.0f;
  float pixelSize = 0.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_GLYPH_HPP
