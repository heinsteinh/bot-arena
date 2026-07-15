#ifndef ENGINE_RENDERER_TEXT_FONTMANAGER_HPP
#define ENGINE_RENDERER_TEXT_FONTMANAGER_HPP

#include <functional>
#include <unordered_map>

#include "engine/core/Base.hpp"
#include "engine/renderer/text/FontAsset.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/FontSource.hpp"
#include "engine/renderer/text/GlyphAtlas.hpp"

namespace engine {

// Builds a GlyphAtlas from a BakedFont. Injected so the manager stays free of
// GL (production passes a Texture2D-backed factory; tests pass a stub).
using AtlasFactory = std::function<Ref<GlyphAtlas>(const BakedFont&)>;

// Sole owner/cache of fonts (ADR-5). Load by FontDesc; identical descriptors
// share one FontAsset. Sources register per backend.
class FontManager {
 public:
  explicit FontManager(AtlasFactory atlasFactory)
      : m_atlasFactory(std::move(atlasFactory)) {}

  void registerSource(Scope<FontSource> source);
  FontHandle load(const FontDesc& desc);

 private:
  AtlasFactory m_atlasFactory;
  std::unordered_map<FontBackend, Scope<FontSource>> m_sources;
  std::unordered_map<FontDesc, FontHandle, FontDescHash> m_cache;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_FONTMANAGER_HPP
