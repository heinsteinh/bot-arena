#ifndef ENGINE_RENDERER_TEXT_TEXTPLACEMENT_HPP
#define ENGINE_RENDERER_TEXT_TEXTPLACEMENT_HPP

#include <glm/glm.hpp>

namespace engine {

enum class PlacementMode {
  ScreenSpace,      // pixel coords, y-down (implemented this slice)
  WorldOriented,    // reserved
  CameraBillboard,  // reserved
  AxisBillboard,    // reserved
};

// How local text coordinates map into the target surface. Screen-space uses
// (pos = baseline pixel origin, scale = px multiplier). World fields are added
// in the world-text slice without disturbing the screen path.
struct TextPlacement {
  PlacementMode mode = PlacementMode::ScreenSpace;
  glm::vec2 pos{0.0f};
  float scale = 1.0f;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTPLACEMENT_HPP
