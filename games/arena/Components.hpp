#ifndef GAMES_ARENA_COMPONENTS_HPP
#define GAMES_ARENA_COMPONENTS_HPP

#include <glm/glm.hpp>

namespace arena {

struct Velocity {
  glm::vec3 value{0.0f};
};

struct Health {
  float current = 0.0f;
  float max = 0.0f;
};

struct Player {};  // tag
struct Bot {};     // tag

}  // namespace arena

#endif  // GAMES_ARENA_COMPONENTS_HPP
