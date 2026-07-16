#ifndef ENGINE_RENDERER_RESOURCEREGISTRY_HPP
#define ENGINE_RENDERER_RESOURCEREGISTRY_HPP

#include <glm/glm.hpp>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/renderer/RenderCommand.hpp"  // handle typedefs
#include "engine/renderer/Texture2D.hpp"

namespace engine {

class Shader;
class VertexArray;

struct Material {
  glm::vec4 baseColor{1.0f};
  float metallic = 0.0f;
  float roughness = 0.5f;
  ShaderHandle shader = 0;
  Ref<Texture2D> albedo = nullptr;
  Ref<Texture2D> normalMap = nullptr;  // null -> use the vertex normal
};

class ResourceRegistry {
 public:
  ShaderHandle registerShader(const Ref<Shader>& shader);
  MeshHandle registerMesh(const Ref<VertexArray>& mesh);
  MaterialHandle registerMaterial(const Material& material);

  const Ref<Shader>& shader(ShaderHandle handle) const;
  const Ref<VertexArray>& mesh(MeshHandle handle) const;
  const Material& material(MaterialHandle handle) const;

 private:
  std::vector<Ref<Shader>> m_shaders;
  std::vector<Ref<VertexArray>> m_meshes;
  std::vector<Material> m_materials;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RESOURCEREGISTRY_HPP
