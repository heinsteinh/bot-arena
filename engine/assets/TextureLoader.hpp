#ifndef ENGINE_ASSETS_TEXTURELOADER_HPP
#define ENGINE_ASSETS_TEXTURELOADER_HPP

#include <string>

#include "engine/core/Base.hpp"
#include "engine/renderer/Texture2D.hpp"

namespace engine {

// Load an image file (jpg/png/...) into an RGBA8 texture. Null on failure.
Ref<Texture2D> loadTexture(const std::string& path);

}  // namespace engine

#endif  // ENGINE_ASSETS_TEXTURELOADER_HPP
