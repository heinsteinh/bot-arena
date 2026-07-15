#ifndef ENGINE_RENDERER_TEXT_TEXTVERTEX_HPP
#define ENGINE_RENDERER_TEXT_TEXTVERTEX_HPP

#include <cstdint>
#include <glm/glm.hpp>

namespace engine {

// GPU vertex for a glyph corner. Fixed now to serve bitmap/SDF/MSDF, single
// color/rich text, and screen/world. 32 bytes, tightly packed; uploaded
// directly to the text VBO. z is reserved (0 for screen space).
struct TextVertex {
  glm::vec3 pos;          // NDC (screen) or world-clip (later)
  glm::vec2 uv;           // atlas UV
  uint32_t fillColor;     // packed RGBA8
  uint32_t outlineColor;  // packed RGBA8 (reserved)
  uint32_t styleIndex;    // reserved: per-batch style-table index
};

// RGBA8, red in the low byte -> matches GLSL unpackUnorm4x8.
inline uint32_t packColor(const glm::vec4& c) {
  const auto to8 = [](float v) -> uint32_t {
    const float x = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    return static_cast<uint32_t>(x * 255.0f + 0.5f);
  };
  return to8(c.r) | (to8(c.g) << 8) | (to8(c.b) << 16) | (to8(c.a) << 24);
}

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTVERTEX_HPP
