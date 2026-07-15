#ifndef ENGINE_RENDERER_TEXT_FONTASSET_HPP
#define ENGINE_RENDERER_TEXT_FONTASSET_HPP

#include <cstdint>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/Glyph.hpp"
#include "engine/renderer/text/GlyphAtlas.hpp"

namespace engine {

// Backend-tagged font handle (ADR-1: data, not a vtable). Callers hold this via
// FontHandle; the renderer batches by (backend, atlas) and never subclasses it.
struct FontAsset {
  FontBackend backend = FontBackend::Bitmap;
  Ref<GlyphAtlas> atlas;
  GlyphStore glyphs;
  FaceMetrics metrics;
  float pxRange = 0.0f;                  // 0 for bitmap; SDF/MSDF set it later
  std::vector<Ref<FontAsset>> fallback;  // empty this slice

  uint32_t atlasRendererID() const { return atlas->rendererID(); }
};

using FontHandle = Ref<FontAsset>;

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_FONTASSET_HPP
