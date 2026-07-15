#ifndef ENGINE_RENDERER_TEXT_TEXTRENDERER_HPP
#define ENGINE_RENDERER_TEXT_TEXTRENDERER_HPP

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/renderer/text/FontAsset.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/renderer/text/TextVertex.hpp"

namespace engine {

// Turns text submissions into GPU-ready vertex batches. Layout is local
// (TextLayout); placement projection is applied here per surface (ADR-2). This
// slice implements ScreenSpace only. Batches merge by (backend, atlas).
class TextRenderer {
 public:
  struct Batch {
    uint32_t atlas = 0;
    FontBackend backend = FontBackend::Bitmap;
    float pxRange = 0.0f;
    std::vector<TextVertex> verts;
  };

  void submit(const FontAsset& font, std::string_view text,
              const TextPlacement& placement, const TextStyle& style,
              int screenW, int screenH);

  const std::vector<Batch>& batches() const { return m_batches; }
  void clear();

 private:
  Batch& batchFor(FontBackend backend, uint32_t atlas, float pxRange);

  std::vector<Batch> m_batches;
  std::unordered_map<uint64_t, size_t> m_index;  // key -> batch index
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTRENDERER_HPP
