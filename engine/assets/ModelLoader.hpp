#ifndef ENGINE_ASSETS_MODELLOADER_HPP
#define ENGINE_ASSETS_MODELLOADER_HPP

#include <string>

#include "engine/assets/Model.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

// Load a mesh file via Assimp: one position+normal mesh per sub-mesh, each with
// a material coloured from the file's diffuse colour. Returns {valid=false} on
// failure. `shader` is the mesh shader the materials render with.
Model loadModel(const std::string& path, ResourceRegistry& registry,
                ShaderHandle shader);

}  // namespace engine

#endif  // ENGINE_ASSETS_MODELLOADER_HPP
