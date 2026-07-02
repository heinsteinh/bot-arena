#ifndef GAMES_ARENA_COMPONENTS_HPP
#define GAMES_ARENA_COMPONENTS_HPP

#include <glm/glm.hpp>

namespace arena {

struct Transform {
  glm::vec3 position{0.0f};
  float scale{0.3f};
};

struct Velocity {
  glm::vec3 value{0.0f};
};

struct Player {};  // tag
struct Bot {};     // tag

}  // namespace arena

#endif  // GAMES_ARENA_COMPONENTS_HPP
