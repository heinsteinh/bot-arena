#ifndef ENGINE_SCENE_LIGHTCOMPONENT_HPP
#define ENGINE_SCENE_LIGHTCOMPONENT_HPP

#include <glm/glm.hpp>

namespace engine {

enum class LightType { Directional, Point };

// Data-only scene light. Point lights use the owning entity's
// TransformComponent.translation as their world position and `radius` as their
// falloff radius; directional lights use normalize(translation) as the
// "toward-light" direction and ignore `radius`.
struct LightComponent {
  LightType type = LightType::Point;
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 1.0f;
  float radius = 10.0f;  // point lights only; ignored for directional
};

}  // namespace engine

#endif  // ENGINE_SCENE_LIGHTCOMPONENT_HPP
