#ifndef ENGINE_RENDERER_TEXT_FONTSOURCE_HPP
#define ENGINE_RENDERER_TEXT_FONTSOURCE_HPP

#include <cstdint>
#include <vector>

#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/Glyph.hpp"

namespace engine {

// CPU result of baking a font: R8 atlas pixels + metrics. No GL here so sources
// stay testable and the manager owns texture upload.
struct BakedFont {
  std::vector<uint8_t> atlasPixels;  // R8, row-major, atlasWidth*atlasHeight
  int atlasWidth = 0;
  int atlasHeight = 0;
  GlyphStore glyphs;
  FaceMetrics metrics;
  float pxRange = 0.0f;
};

// Produces a BakedFont for a descriptor. Concrete sources register with the
// FontManager (ADR-5), so engine core depends only on this interface.
class FontSource {
 public:
  virtual ~FontSource() = default;
  virtual FontBackend backend() const = 0;
  virtual bool bake(const FontDesc& desc, BakedFont& out) = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_FONTSOURCE_HPP
