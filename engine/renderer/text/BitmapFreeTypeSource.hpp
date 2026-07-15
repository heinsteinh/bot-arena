#ifndef ENGINE_RENDERER_TEXT_BITMAPFREETYPESOURCE_HPP
#define ENGINE_RENDERER_TEXT_BITMAPFREETYPESOURCE_HPP

#include "engine/renderer/text/FontSource.hpp"

namespace engine {

// Bakes an anti-aliased R8 bitmap atlas with FreeType (the classic path).
class BitmapFreeTypeSource : public FontSource {
 public:
  FontBackend backend() const override { return FontBackend::Bitmap; }
  bool bake(const FontDesc& desc, BakedFont& out) override;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_BITMAPFREETYPESOURCE_HPP
