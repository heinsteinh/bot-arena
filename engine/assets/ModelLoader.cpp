#include "engine/assets/ModelLoader.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <cstdint>
#include <vector>

#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

Model loadModel(const std::string& path, ResourceRegistry& registry) {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                aiProcess_JoinIdenticalVertices |
                aiProcess_PreTransformVertices);
  if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
      !scene->mRootNode) {
    spdlog::error("Failed to load model {}: {}", path,
                  importer.GetErrorString());
    return Model{};
  }

  std::vector<float> verts;       // interleaved px,py,pz,nx,ny,nz
  std::vector<uint32_t> indices;  // triangles
  std::vector<glm::vec3> positions;
  for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
    const aiMesh* mesh = scene->mMeshes[m];
    const uint32_t base = static_cast<uint32_t>(positions.size());
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
      const aiVector3D& p = mesh->mVertices[i];
      const aiVector3D n =
          mesh->mNormals ? mesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
      verts.insert(verts.end(), {p.x, p.y, p.z, n.x, n.y, n.z});
      positions.push_back({p.x, p.y, p.z});
    }
    for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
      const aiFace& face = mesh->mFaces[f];
      for (unsigned j = 0; j < face.mNumIndices; ++j) {
        indices.push_back(base + face.mIndices[j]);
      }
    }
  }

  if (positions.empty() || indices.empty()) {
    spdlog::error("Model {} has no geometry", path);
    return Model{};
  }

  auto va = VertexArray::Create();
  auto vb = VertexBuffer::Create(
      verts.data(), static_cast<uint32_t>(verts.size() * sizeof(float)));
  vb->setLayout({
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float3, "a_normal"},
  });
  va->addVertexBuffer(vb);
  va->setIndexBuffer(IndexBuffer::Create(
      indices.data(), static_cast<uint32_t>(indices.size())));

  Model model;
  model.mesh = registry.registerMesh(va);
  model.bounds = computeBounds(positions.data(), positions.size());
  model.valid = true;
  spdlog::info("Loaded model {} ({} verts, {} tris)", path, positions.size(),
               indices.size() / 3);
  return model;
}

}  // namespace engine
