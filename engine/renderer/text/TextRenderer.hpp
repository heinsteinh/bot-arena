#ifndef ENGINE_RENDERER_TEXT_TEXTRENDERER_HPP
#define ENGINE_RENDERER_TEXT_TEXTRENDERER_HPP

#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "engine/renderer/text/FontAsset.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextSpan.hpp"
#include "engine/renderer/text/TextStyle.hpp"
#include "engine/renderer/text/TextVertex.hpp"
#include "engine/renderer/text/WorldTextVertex.hpp"

namespace engine {

// Turns text submissions into GPU-ready vertex batches. Layout is local
// (TextLayout); placement projection is applied here per surface (ADR-2). This
// slice implements ScreenSpace only. Batches merge by (backend, atlas).
class TextRenderer {
 public:
  static constexpr uint32_t kMaxStylesPerBatch = 64;

  struct Batch {
    uint32_t atlas = 0;
    FontBackend backend = FontBackend::Bitmap;
    float pxRange = 0.0f;
    std::vector<GpuStyle> styles;
    std::vector<TextVertex> verts;
  };

  void submit(const FontAsset& font, std::string_view text,
              const TextPlacement& placement, const TextStyle& style,
              int screenW, int screenH);

  void submit(const FontAsset& font, std::span<const TextSpan> spans,
              const TextPlacement& placement, int screenW, int screenH);

  struct WorldBatch {
    uint32_t atlas = 0;
    FontBackend backend = FontBackend::Bitmap;
    float pxRange = 0.0f;
    std::vector<GpuStyle> styles;
    std::vector<WorldTextVertex> verts;
  };

  const std::vector<Batch>& batches() const { return m_batches; }
  const std::vector<WorldBatch>& worldBatches() const { return m_worldBatches; }
  void clear();

 private:
  // Returns the batch index for (backend, atlas) that can host `style`, plus
  // the style's index within that batch's table. Splits to a new batch past the
  // cap.
  std::pair<std::size_t, uint32_t> acquire(FontBackend backend, uint32_t atlas,
                                           float pxRange,
                                           const GpuStyle& style);

  template <class BatchT>
  std::pair<std::size_t, uint32_t> acquireIn(
      std::vector<BatchT>& batches,
      std::unordered_map<uint64_t, std::vector<std::size_t>>& index,
      FontBackend backend, uint32_t atlas, float pxRange,
      const GpuStyle& style);

  void submitScreen(const FontAsset& font, std::span<const TextSpan> spans,
                    const TextPlacement& placement, int screenW, int screenH);
  void submitBillboard(const FontAsset& font, std::span<const TextSpan> spans,
                       const TextPlacement& placement);

  std::vector<Batch> m_batches;
  std::unordered_map<uint64_t, std::vector<std::size_t>>
      m_index;  // key -> batches

  std::vector<WorldBatch> m_worldBatches;
  std::unordered_map<uint64_t, std::vector<std::size_t>> m_worldIndex;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTRENDERER_HPP
