#ifndef ENGINE_CORE_ASSETPATH_HPP
#define ENGINE_CORE_ASSETPATH_HPP

#include <string>

#ifndef BOTARENA_ASSET_DIR
#define BOTARENA_ASSET_DIR "assets"
#endif

namespace engine {

inline std::string assetPath(const std::string& relative) {
  return std::string(BOTARENA_ASSET_DIR) + "/" + relative;
}

}  // namespace engine

#endif  // ENGINE_CORE_ASSETPATH_HPP
