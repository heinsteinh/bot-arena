#ifndef ENGINE_RENDERER_SSAOKERNEL_HPP
#define ENGINE_RENDERER_SSAOKERNEL_HPP

#include <cmath>
#include <glm/glm.hpp>
#include <vector>

namespace engine {

// A hemisphere SSAO sample kernel (view-space, +Z). Deterministic: a fixed hash
// stands in for rand() so results are stable and unit-testable.
inline std::vector<glm::vec3> generateSSAOKernel(int count) {
  auto hash = [](int i, int salt) {
    float x = std::sin(static_cast<float>(i) * 12.9898f +
                       static_cast<float>(salt) * 78.233f) *
              43758.5453f;
    return x - std::floor(x);  // fract, in [0,1)
  };
  std::vector<glm::vec3> kernel;
  kernel.reserve(static_cast<size_t>(count));
  for (int i = 0; i < count; ++i) {
    glm::vec3 sample(hash(i, 0) * 2.0f - 1.0f, hash(i, 1) * 2.0f - 1.0f,
                     hash(i, 2));  // z in [0,1) -> hemisphere
    sample = glm::normalize(sample);
    sample *= hash(i, 3);  // scatter within the hemisphere
    const float t = static_cast<float>(i) / static_cast<float>(count);
    const float scale = 0.1f + 0.9f * t * t;  // bias toward the origin
    kernel.push_back(sample * scale);
  }
  return kernel;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_SSAOKERNEL_HPP
