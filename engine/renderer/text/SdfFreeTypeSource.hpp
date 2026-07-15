#ifndef ENGINE_RENDERER_TEXT_SDFFREETYPESOURCE_HPP
#define ENGINE_RENDERER_TEXT_SDFFREETYPESOURCE_HPP

#include "engine/renderer/text/FontSource.hpp"

namespace engine {

// Bakes a single-channel R8 signed-distance atlas with FreeType's outline SDF
// renderer (FT_RENDER_MODE_SDF). Crisp at any scale; substrate for effects.
class SdfFreeTypeSource : public FontSource {
 public:
  FontBackend backend() const override { return FontBackend::SDF; }
  bool bake(const FontDesc& desc, BakedFont& out) override;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_SDFFREETYPESOURCE_HPP
