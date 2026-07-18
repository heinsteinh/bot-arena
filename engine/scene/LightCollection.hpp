#ifndef ENGINE_SCENE_LIGHTCOLLECTION_HPP
#define ENGINE_SCENE_LIGHTCOLLECTION_HPP

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "engine/renderer/PointLight.hpp"
#include "engine/scene/Components.hpp"
#include "engine/scene/LightComponent.hpp"

namespace engine {

// Result of walking a registry's light entities. Additive by contract: an
// empty `points` / false `hasDirectional` means "this scene declares none",
// not "clear the renderer's lights".
struct CollectedLights {
  std::vector<PointLight> points;
  bool hasDirectional = false;
  glm::vec3 directionalDir = glm::vec3(0.0f, 1.0f, 0.0f);
};

// Collect all <TransformComponent, LightComponent> entities into renderer-ready
// data. Point lights become PointLights (position = translation, radius =
// LightComponent.radius, color.rgb = color, color.a = intensity). The first
// directional light in view order supplies directionalDir = normalize(
// translation); later directional lights are ignored (the renderer honors one).
inline CollectedLights collectLights(const entt::registry& reg) {
  CollectedLights out;
  auto view = reg.view<const TransformComponent, const LightComponent>();
  for (const entt::entity e : view) {
    const TransformComponent& t = view.get<const TransformComponent>(e);
    const LightComponent& lc = view.get<const LightComponent>(e);
    if (lc.type == LightType::Point) {
      PointLight pl;
      pl.positionRadius = glm::vec4(t.translation, lc.radius);
      pl.color = glm::vec4(lc.color, lc.intensity);
      out.points.push_back(pl);
    } else if (!out.hasDirectional) {
      out.hasDirectional = true;
      out.directionalDir = glm::normalize(t.translation);
    }
  }
  return out;
}

}  // namespace engine

#endif  // ENGINE_SCENE_LIGHTCOLLECTION_HPP
