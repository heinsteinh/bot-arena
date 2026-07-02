#ifndef ENGINE_RENDERER_PARTICLEINSTANCE_HPP
#define ENGINE_RENDERER_PARTICLEINSTANCE_HPP

#include <glm/glm.hpp>

namespace engine {

// Per-particle GPU instance data for the additive billboard pass.
struct ParticleInstance {
  glm::vec3 position;
  float size;
  glm::vec4 color;  // rgb pre-faded; a = 1
};

}  // namespace engine

#endif  // ENGINE_RENDERER_PARTICLEINSTANCE_HPP
