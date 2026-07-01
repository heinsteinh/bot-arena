#ifndef ENGINE_RENDERER_MINIMAPRECT_HPP
#define ENGINE_RENDERER_MINIMAPRECT_HPP

#include <glm/glm.hpp>

namespace engine {

// Top-right inset NDC rect {x0,y0,x1,y1}. Height is `heightFrac` of the screen;
// width compensates for aspect so a square source stays square on screen.
inline glm::vec4 minimapRect(int screenW, int screenH, float heightFrac,
                             float margin) {
  const float aspect =
      static_cast<float>(screenW) / static_cast<float>(screenH);
  const float hNdc = heightFrac * 2.0f;  // fraction of full height -> NDC
  const float wNdc = hNdc / aspect;      // keep square on screen
  const float x1 = 1.0f - margin;
  const float y1 = 1.0f - margin;
  const float x0 = x1 - wNdc;
  const float y0 = y1 - hNdc;
  return {x0, y0, x1, y1};
}

}  // namespace engine

#endif  // ENGINE_RENDERER_MINIMAPRECT_HPP
