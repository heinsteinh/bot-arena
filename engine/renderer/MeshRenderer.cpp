#include "engine/renderer/MeshRenderer.hpp"

#include <algorithm>

namespace engine {

void MeshRenderer::submit(MeshHandle mesh, MaterialHandle material,
                          const glm::mat4& transform) {
  const Material& mat = m_registry.material(material);

  const glm::vec3 translation = glm::vec3(transform[3]);
  const float viewZ =
      (m_camera.view() * glm::vec4(translation, 1.0f)).z;  // <0 in front
  const float normalized = std::clamp(-viewZ / kFarPlane, 0.0f, 1.0f);
  const uint32_t depth24 = static_cast<uint32_t>(normalized * 0x00FFFFFF);

  RenderCommand cmd;
  cmd.type = RenderCommandType::Mesh;
  cmd.layer = RenderLayer::Opaque;
  cmd.mesh = mesh;
  cmd.material = material;
  cmd.transform = transform;
  cmd.sortKey = makeSortKey(RenderLayer::Opaque, mat.shader, material, depth24);
  m_queue.submit(cmd);
}

}  // namespace engine
