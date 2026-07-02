#ifndef ENGINE_ASSETS_TEXTUREPATH_HPP
#define ENGINE_ASSETS_TEXTUREPATH_HPP

#include <algorithm>
#include <string>

namespace engine {

// Resolve a material's texture reference against the model file's directory.
// Absolute references pass through; relative ones join the model directory.
inline std::string resolveTexturePath(const std::string& modelPath,
                                      const std::string& textureRef) {
  std::string ref = textureRef;
  std::replace(ref.begin(), ref.end(), '\\', '/');
  if (!ref.empty() && ref[0] == '/') return ref;
  const std::size_t slash = modelPath.find_last_of('/');
  if (slash == std::string::npos) return ref;
  return modelPath.substr(0, slash) + "/" + ref;
}

}  // namespace engine

#endif  // ENGINE_ASSETS_TEXTUREPATH_HPP
