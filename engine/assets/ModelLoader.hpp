#ifndef ENGINE_ASSETS_MODELLOADER_HPP
#define ENGINE_ASSETS_MODELLOADER_HPP

#include <string>

#include "engine/assets/MeshBounds.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

struct Model {
  MeshHandle mesh = 0;
  AABB bounds{};
  bool valid = false;
};

// Load a mesh file (obj/etc.) via Assimp into a position+normal mesh registered
// with `registry`. Returns {valid=false} on failure.
Model loadModel(const std::string& path, ResourceRegistry& registry);

}  // namespace engine

#endif  // ENGINE_ASSETS_MODELLOADER_HPP
