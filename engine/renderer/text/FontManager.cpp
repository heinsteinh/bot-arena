#include "engine/renderer/text/FontManager.hpp"

#include <spdlog/spdlog.h>

#include <utility>

namespace engine {

void FontManager::registerSource(Scope<FontSource> source) {
  const FontBackend backend = source->backend();
  m_sources[backend] = std::move(source);
}

FontHandle FontManager::load(const FontDesc& desc) {
  if (auto it = m_cache.find(desc); it != m_cache.end()) {
    return it->second;
  }
  auto srcIt = m_sources.find(desc.backend);
  if (srcIt == m_sources.end()) {
    spdlog::error("FontManager: no source registered for backend {}",
                  static_cast<int>(desc.backend));
    return nullptr;
  }
  BakedFont baked;
  if (!srcIt->second->bake(desc, baked)) {
    spdlog::error("FontManager: bake failed for '{}'", desc.family);
    return nullptr;
  }
  FontHandle asset = CreateRef<FontAsset>();
  asset->backend = desc.backend;
  asset->glyphs = std::move(baked.glyphs);
  asset->metrics = baked.metrics;
  asset->pxRange = baked.pxRange;
  asset->atlas = m_atlasFactory(baked);
  m_cache.emplace(desc, asset);
  return asset;
}

}  // namespace engine
