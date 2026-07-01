#ifndef ENGINE_RENDERER_POINTLIGHT_HPP
#define ENGINE_RENDERER_POINTLIGHT_HPP

#include <cmath>
#include <glm/glm.hpp>

namespace engine {

// std140-compatible point light: xyz position + w radius; rgb colour + a
// intensity.
struct PointLight {
  glm::vec4 positionRadius{0.0f};
  glm::vec4 color{1.0f};
};

// Position of light `index` of `count` on a ring of `radius` at `height`,
// rigidly rotated by `time` (radians).
inline glm::vec3 pointLightRing(int index, int count, float time, float radius,
                                float height) {
  const float twoPi = 6.2831853071795864769f;
  const float angle =
      time + twoPi * static_cast<float>(index) / static_cast<float>(count);
  return glm::vec3(radius * std::cos(angle), height, radius * std::sin(angle));
}

}  // namespace engine

#endif  // ENGINE_RENDERER_POINTLIGHT_HPP
