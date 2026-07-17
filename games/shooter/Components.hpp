#ifndef GAMES_SHOOTER_COMPONENTS_HPP
#define GAMES_SHOOTER_COMPONENTS_HPP

#include <glm/glm.hpp>

namespace shooter {

struct Velocity {
  glm::vec3 value{0.0f};
};

struct Health {
  float current = 0.0f;
  float max = 0.0f;
};

struct Player {};  // tag

struct Enemy {
  int tier = 0;  // 0 grunt / 1 heavy / 2 elite
  float fireTimer = 0.0f;
};

struct Bullet {
  float life = 0.0f;
  bool fromPlayer = true;
};

}  // namespace shooter

#endif  // GAMES_SHOOTER_COMPONENTS_HPP
