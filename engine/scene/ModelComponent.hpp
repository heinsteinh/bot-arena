#ifndef ENGINE_SCENE_MODELCOMPONENT_HPP
#define ENGINE_SCENE_MODELCOMPONENT_HPP

#include <glm/gtc/quaternion.hpp>  // glm::quat

#include "engine/renderer/RenderCommand.hpp"  // ModelHandle, MaterialHandle

namespace engine {

// Draws a whole multi-submesh Model (looked up by handle in ResourceRegistry)
// at the entity's TransformComponent. Kept in its own header so Components.hpp
// stays free of renderer/asset dependencies.
struct ModelComponent {
  ModelHandle model = 0;
  bool normalized = true;  // fit bounds into a unit cube
  MaterialHandle materialOverride =
      0;  // 0 = baked submesh materials; else tint all
  // Render-only facing correction (identity by default). Applied as
  // rotation * rotationOffset so the gameplay TransformComponent.rotation
  // stays the pure heading (games read it back to aim). Models that import
  // facing the wrong way set this (e.g. angleAxis(pi, +Y)).
  glm::quat rotationOffset{1.0f, 0.0f, 0.0f, 0.0f};
};

}  // namespace engine

#endif  // ENGINE_SCENE_MODELCOMPONENT_HPP
