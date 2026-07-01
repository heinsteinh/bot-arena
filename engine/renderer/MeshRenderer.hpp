#ifndef ENGINE_RENDERER_MESHRENDERER_HPP
#define ENGINE_RENDERER_MESHRENDERER_HPP

#include <glm/glm.hpp>

#include "engine/renderer/Camera.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/RenderQueue.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

class MeshRenderer {
 public:
  static constexpr float kFarPlane = 100.0f;

  MeshRenderer(RenderQueue& queue, const ResourceRegistry& registry,
               const Camera& camera)
      : m_queue(queue), m_registry(registry), m_camera(camera) {}

  void submit(MeshHandle mesh, MaterialHandle material,
              const glm::mat4& transform);

 private:
  RenderQueue& m_queue;
  const ResourceRegistry& m_registry;
  const Camera& m_camera;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_MESHRENDERER_HPP
