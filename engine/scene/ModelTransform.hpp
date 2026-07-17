#ifndef ENGINE_SCENE_MODELTRANSFORM_HPP
#define ENGINE_SCENE_MODELTRANSFORM_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "engine/assets/MeshBounds.hpp"     // AABB, fitToUnitTransform
#include "engine/scene/Components.hpp"      // TransformComponent
#include "engine/scene/ModelComponent.hpp"  // ModelComponent

namespace engine {

// World matrix for every submesh of a ModelComponent entity. The gameplay
// rotation is composed with the render-only facing offset, then (when
// normalized) the model is fit into a unit cube from its bounds.
inline glm::mat4 modelRenderTransform(const TransformComponent& t,
                                      const ModelComponent& mc,
                                      const AABB& bounds) {
  const glm::quat rot = t.rotation * mc.rotationOffset;
  glm::mat4 m = glm::translate(glm::mat4(1.0f), t.translation) *
                glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), t.scale);
  if (mc.normalized) m = m * fitToUnitTransform(bounds);
  return m;
}

}  // namespace engine

#endif  // ENGINE_SCENE_MODELTRANSFORM_HPP
