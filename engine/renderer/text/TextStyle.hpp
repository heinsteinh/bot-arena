#ifndef ENGINE_RENDERER_TEXT_TEXTSTYLE_HPP
#define ENGINE_RENDERER_TEXT_TEXTSTYLE_HPP

#include <cstdint>
#include <glm/glm.hpp>

namespace engine {

// Visual style for a text run. Only fillColor is honored by the bitmap
// backend this slice; the rest are reserved for the SDF/MSDF effect stage.
struct TextStyle {
  glm::vec4 fillColor{1.0f};
  glm::vec4 outlineColor{0.0f};  // reserved
  float outlineWidth = 0.0f;     // reserved (distance units)
  uint32_t styleIndex = 0;       // reserved: index into per-batch style table
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTSTYLE_HPP
