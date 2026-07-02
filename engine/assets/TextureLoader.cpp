#include "engine/assets/TextureLoader.hpp"

#include <spdlog/spdlog.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace engine {

Ref<Texture2D> loadTexture(const std::string& path) {
  stbi_set_flip_vertically_on_load(true);
  int w = 0, h = 0, channels = 0;
  stbi_uc* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
  if (!pixels) {
    spdlog::error("Failed to load texture {}: {}", path, stbi_failure_reason());
    return nullptr;
  }
  Ref<Texture2D> tex = Texture2D::Create(
      static_cast<uint32_t>(w), static_cast<uint32_t>(h), TextureFormat::RGBA8);
  tex->setData(pixels, static_cast<uint32_t>(w * h * 4));
  stbi_image_free(pixels);
  spdlog::info("Loaded texture {} ({}x{})", path, w, h);
  return tex;
}

}  // namespace engine
