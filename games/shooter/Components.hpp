#ifndef GAMES_SHOOTER_COMPONENTS_HPP
#define GAMES_SHOOTER_COMPONENTS_HPP

#include <glm/glm.hpp>

namespace shooter {

struct Transform {
  glm::vec3 position{0.0f};
  float scale{0.5f};
  float yaw{0.0f};
};

struct Velocity {
  glm::vec3 value{0.0f};
};

struct Player {};  // tag

struct Enemy {
  int tier = 0;  // 0 grunt / 1 heavy / 2 elite
};

struct Bullet {
  float life = 0.0f;
};

}  // namespace shooter

#endif  // GAMES_SHOOTER_COMPONENTS_HPP
