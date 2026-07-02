#ifndef ENGINE_ASSETS_MESHBOUNDS_HPP
#define ENGINE_ASSETS_MESHBOUNDS_HPP

#include <algorithm>
#include <cstddef>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

struct AABB {
  glm::vec3 min{0.0f};
  glm::vec3 max{0.0f};
};

inline AABB computeBounds(const glm::vec3* points, std::size_t count) {
  if (count == 0 || points == nullptr) return AABB{};
  AABB b;
  b.min = points[0];
  b.max = points[0];
  for (std::size_t i = 1; i < count; ++i) {
    b.min = glm::min(b.min, points[i]);
    b.max = glm::max(b.max, points[i]);
  }
  return b;
}

inline glm::mat4 fitToUnitTransform(const AABB& b) {
  const glm::vec3 center = (b.min + b.max) * 0.5f;
  const glm::vec3 extent = b.max - b.min;
  const float largest = std::max(extent.x, std::max(extent.y, extent.z));
  const float s = largest > 1e-6f ? 1.0f / largest : 1.0f;
  return glm::scale(glm::mat4(1.0f), glm::vec3(s)) *
         glm::translate(glm::mat4(1.0f), -center);
}

}  // namespace engine

#endif  // ENGINE_ASSETS_MESHBOUNDS_HPP
