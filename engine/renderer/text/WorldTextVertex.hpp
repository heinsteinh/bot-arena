#ifndef ENGINE_RENDERER_TEXT_WORLDTEXTVERTEX_HPP
#define ENGINE_RENDERER_TEXT_WORLDTEXTVERTEX_HPP

#include <cstdint>
#include <glm/glm.hpp>

namespace engine {

// GPU vertex for a billboard glyph corner (SDF-only; colors come from the style
// table via styleIndex). 32 bytes, tightly packed.
struct WorldTextVertex {
  glm::vec3 anchor;     // world anchor (same for every glyph of a run)
  glm::vec2 offset;     // world-unit offset along camera right/up
  glm::vec2 uv;         // atlas UV
  uint32_t styleIndex;  // per-batch style-table index
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_WORLDTEXTVERTEX_HPP
