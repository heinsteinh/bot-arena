#ifndef ENGINE_RENDERER_TEXT_TEXTSTYLE_HPP
#define ENGINE_RENDERER_TEXT_TEXTSTYLE_HPP

#include <cstdint>
#include <glm/glm.hpp>

namespace engine {

// Visual style for a text run. Effect fields default to zero (off), so a
// fill-only style renders identically to v0.28/29. Effect sizes are in screen
// pixels; the SDF shader makes them scale-correct. The bitmap backend honors
// only fillColor.
struct TextStyle {
  glm::vec4 fillColor{1.0f};
  glm::vec4 outlineColor{0.0f};
  float outlineWidthPx = 0.0f;
  glm::vec4 glowColor{0.0f};
  float glowSizePx = 0.0f;
  glm::vec4 shadowColor{0.0f};
  glm::vec2 shadowOffsetPx{0.0f};
  float shadowSoftnessPx = 0.0f;
  uint32_t styleIndex =
      0;  // internal: assigned by TextRenderer, not the caller
};

// GPU mirror of a style, laid out for a std140 UBO array (6 x vec4 = 96 bytes).
struct GpuStyle {
  glm::vec4 fillColor{1.0f};
  glm::vec4 outlineColor{0.0f};
  glm::vec4 glowColor{0.0f};
  glm::vec4 shadowColor{0.0f};
  glm::vec4 params0{0.0f};  // outlineWidthPx, glowSizePx, shadowOffsetPx.x, .y
  glm::vec4 params1{0.0f};  // shadowSoftnessPx, unused, unused, unused

  bool operator==(const GpuStyle& o) const {
    return fillColor == o.fillColor && outlineColor == o.outlineColor &&
           glowColor == o.glowColor && shadowColor == o.shadowColor &&
           params0 == o.params0 && params1 == o.params1;
  }
};

inline GpuStyle toGpuStyle(const TextStyle& s) {
  GpuStyle g;
  g.fillColor = s.fillColor;
  g.outlineColor = s.outlineColor;
  g.glowColor = s.glowColor;
  g.shadowColor = s.shadowColor;
  g.params0 = {s.outlineWidthPx, s.glowSizePx, s.shadowOffsetPx.x,
               s.shadowOffsetPx.y};
  g.params1 = {s.shadowSoftnessPx, 0.0f, 0.0f, 0.0f};
  return g;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTSTYLE_HPP
