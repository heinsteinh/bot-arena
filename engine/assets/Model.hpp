#ifndef ENGINE_ASSETS_MODEL_HPP
#define ENGINE_ASSETS_MODEL_HPP

#include <vector>

#include "engine/assets/MeshBounds.hpp"        // AABB
#include "engine/renderer/RenderCommand.hpp"   // MeshHandle, MaterialHandle

namespace engine {

struct Submesh {
  MeshHandle mesh = 0;
  MaterialHandle material = 0;
};

struct Model {
  std::vector<Submesh> submeshes;
  AABB bounds{};
  bool valid = false;
};

}  // namespace engine

#endif  // ENGINE_ASSETS_MODEL_HPP
