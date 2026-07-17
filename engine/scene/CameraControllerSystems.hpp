#ifndef ENGINE_SCENE_CAMERACONTROLLERSYSTEMS_HPP
#define ENGINE_SCENE_CAMERACONTROLLERSYSTEMS_HPP

#include <entt/entt.hpp>

namespace engine {

// Each reads the Input global + dt and writes matching cameras'
// TransformComponent.
void updateFlyControllers(entt::registry& registry, float dt);
void updateOrbitControllers(entt::registry& registry, float dt);
void updateFollowControllers(entt::registry& registry, float dt);
void updateCamera2DControllers(entt::registry& registry, float dt);

}  // namespace engine

#endif  // ENGINE_SCENE_CAMERACONTROLLERSYSTEMS_HPP
