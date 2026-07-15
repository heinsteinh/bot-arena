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
  glm::vec2 pos{0.0f};  // screen: baseline pixel origin
  float scale = 1.0f;   // screen: px multiplier | billboard: worldUnitsPerPixel
  glm::vec3 worldPos{0.0f};  // billboard: anchor

  static TextPlacement screen(glm::vec2 pos, float scale = 1.0f) {
    TextPlacement p;
    p.mode = PlacementMode::ScreenSpace;
    p.pos = pos;
    p.scale = scale;
    return p;
  }

  // worldUnitsPerPixel is clamped to a small positive minimum so a non-positive
  // value can never mirror the text.
  static TextPlacement cameraBillboard(glm::vec3 worldPos,
                                       float worldUnitsPerPixel) {
    TextPlacement p;
    p.mode = PlacementMode::CameraBillboard;
    p.worldPos = worldPos;
    p.scale = worldUnitsPerPixel > 1e-5f ? worldUnitsPerPixel : 1e-5f;
    return p;
  }
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTPLACEMENT_HPP
