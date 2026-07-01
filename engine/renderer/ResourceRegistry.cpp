#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

ShaderHandle ResourceRegistry::registerShader(const Ref<Shader>& shader) {
  m_shaders.push_back(shader);
  return static_cast<ShaderHandle>(m_shaders.size() - 1);
}

MeshHandle ResourceRegistry::registerMesh(const Ref<VertexArray>& mesh) {
  m_meshes.push_back(mesh);
  return static_cast<MeshHandle>(m_meshes.size() - 1);
}

MaterialHandle ResourceRegistry::registerMaterial(const Material& material) {
  m_materials.push_back(material);
  return static_cast<MaterialHandle>(m_materials.size() - 1);
}

const Ref<Shader>& ResourceRegistry::shader(ShaderHandle handle) const {
  return m_shaders[handle];
}

const Ref<VertexArray>& ResourceRegistry::mesh(MeshHandle handle) const {
  return m_meshes[handle];
}

const Material& ResourceRegistry::material(MaterialHandle handle) const {
  return m_materials[handle];
}

}  // namespace engine
