#include "engine/assets/ModelLoader.hpp"

#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <cstdint>
#include <vector>

#include "engine/assets/TextureLoader.hpp"
#include "engine/assets/TexturePath.hpp"
#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

namespace {
glm::vec4 diffuseColor(const aiScene* scene, const aiMesh* mesh) {
  glm::vec4 color(0.8f, 0.8f, 0.8f, 1.0f);
  if (mesh->mMaterialIndex < scene->mNumMaterials) {
    const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
    aiColor4D c;
    if (aiGetMaterialColor(mat, AI_MATKEY_COLOR_DIFFUSE, &c) == AI_SUCCESS) {
      color = {c.r, c.g, c.b, 1.0f};
    }
  }
  return color;
}

MeshHandle buildMesh(const aiMesh* mesh, ResourceRegistry& registry) {
  std::vector<float> verts;  // interleaved px,py,pz,nx,ny,nz,u,v
  verts.reserve(mesh->mNumVertices * 8);
  for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
    const aiVector3D& p = mesh->mVertices[i];
    const aiVector3D n =
        mesh->mNormals ? mesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f);
    const aiVector3D uv = mesh->mTextureCoords[0]
                              ? mesh->mTextureCoords[0][i]
                              : aiVector3D(0.0f, 0.0f, 0.0f);
    verts.insert(verts.end(), {p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y});
  }
  std::vector<uint32_t> indices;
  for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
    const aiFace& face = mesh->mFaces[f];
    for (unsigned j = 0; j < face.mNumIndices; ++j) {
      indices.push_back(face.mIndices[j]);
    }
  }

  auto va = VertexArray::Create();
  auto vb = VertexBuffer::Create(
      verts.data(), static_cast<uint32_t>(verts.size() * sizeof(float)));
  vb->setLayout({
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float3, "a_normal"},
      {ShaderDataType::Float2, "a_uv"},
  });
  va->addVertexBuffer(vb);
  va->setIndexBuffer(IndexBuffer::Create(
      indices.data(), static_cast<uint32_t>(indices.size())));
  return registry.registerMesh(va);
}
}  // namespace

Model loadModel(const std::string& path, ResourceRegistry& registry,
                ShaderHandle shader) {
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

  Model model;
  std::vector<glm::vec3> positions;
  uint32_t totalTris = 0;
  for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
    const aiMesh* mesh = scene->mMeshes[m];
    if (mesh->mNumVertices == 0 || mesh->mNumFaces == 0) continue;
    for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
      const aiVector3D& p = mesh->mVertices[i];
      positions.push_back({p.x, p.y, p.z});
    }
    const MeshHandle meshHandle = buildMesh(mesh, registry);

    Material material;
    material.baseColor = diffuseColor(scene, mesh);
    material.metallic = 0.0f;
    material.roughness = 0.55f;
    material.shader = shader;
    if (mesh->mMaterialIndex < scene->mNumMaterials) {
      const aiMaterial* aim = scene->mMaterials[mesh->mMaterialIndex];
      aiString texPath;
      if (aim->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        material.albedo =
            loadTexture(resolveTexturePath(path, texPath.C_Str()));
      }
    }
    const MaterialHandle mat = registry.registerMaterial(material);
    model.submeshes.push_back({meshHandle, mat});
    totalTris += mesh->mNumFaces;
  }

  if (model.submeshes.empty()) {
    spdlog::error("Model {} has no geometry", path);
    return Model{};
  }

  model.bounds = computeBounds(positions.data(), positions.size());
  model.valid = true;
  spdlog::info("Loaded model {} ({} submeshes, {} tris)", path,
               model.submeshes.size(), totalTris);
  return model;
}

}  // namespace engine
