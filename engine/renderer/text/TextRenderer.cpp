#include "engine/renderer/text/TextRenderer.hpp"

#include <span>

#include "engine/renderer/text/TextLayout.hpp"
#include "engine/renderer/text/TextSpan.hpp"

namespace engine {

void TextRenderer::clear() {
  m_batches.clear();
  m_index.clear();
}

std::pair<std::size_t, uint32_t> TextRenderer::acquire(FontBackend backend,
                                                       uint32_t atlas,
                                                       float pxRange,
                                                       const GpuStyle& style) {
  const uint64_t key =
      (static_cast<uint64_t>(backend) << 32) | static_cast<uint64_t>(atlas);
  std::vector<std::size_t>& batches = m_index[key];
  for (std::size_t bi : batches) {
    Batch& b = m_batches[bi];
    for (uint32_t si = 0; si < b.styles.size(); ++si) {
      if (b.styles[si] == style) return {bi, si};
    }
    if (b.styles.size() < kMaxStylesPerBatch) {
      b.styles.push_back(style);
      return {bi, static_cast<uint32_t>(b.styles.size() - 1)};
    }
  }
  const std::size_t bi = m_batches.size();
  m_batches.push_back(Batch{atlas, backend, pxRange, {}, {}});
  m_batches[bi].styles.push_back(style);
  batches.push_back(bi);
  return {bi, 0};
}

void TextRenderer::submit(const FontAsset& font,
                          std::span<const TextSpan> spans,
                          const TextPlacement& placement, int screenW,
                          int screenH) {
  if (placement.mode != PlacementMode::ScreenSpace) return;  // this slice
  if (screenW <= 0 || screenH <= 0) return;

  const float sw = static_cast<float>(screenW);
  const float sh = static_cast<float>(screenH);
  const auto ndcX = [&](float px) { return px / sw * 2.0f - 1.0f; };
  const auto ndcY = [&](float py) { return 1.0f - py / sh * 2.0f; };

  TextLayoutState state;
  std::vector<TextQuad>
      quads;  // shared buffer; state carries the pen across spans
  for (const TextSpan& span : spans) {
    if (span.text.empty()) continue;  // no pen move, no style entry
    const std::size_t first = quads.size();
    appendTextLayout(font.glyphs, span.text, state, quads);
    if (quads.size() == first) continue;  // no visible glyph -> no style entry

    const uint32_t fill = packColor(span.style.fillColor);
    const uint32_t outline = packColor(span.style.outlineColor);
    const auto [batchIdx, styleIdx] =
        acquire(font.backend, font.atlasRendererID(), font.pxRange,
                toGpuStyle(span.style));
    Batch& batch =
        m_batches[batchIdx];  // fetch AFTER acquire (may have reallocated)
    // No per-span reserve: exact-size reserves across many same-batch spans
    // would defeat the vector's amortized growth; push_back amortizes here.

    for (std::size_t qi = first; qi < quads.size(); ++qi) {
      const TextQuad& q = quads[qi];
      const float px0 = placement.pos.x + q.x0 * placement.scale;
      const float py0 = placement.pos.y + q.y0 * placement.scale;
      const float px1 = placement.pos.x + q.x1 * placement.scale;
      const float py1 = placement.pos.y + q.y1 * placement.scale;
      const float x0 = ndcX(px0), x1 = ndcX(px1);
      const float y0 = ndcY(py0), y1 = ndcY(py1);
      const auto V = [&](float x, float y, float u, float v) {
        return TextVertex{{x, y, 0.0f}, {u, v}, fill, outline, styleIdx};
      };
      batch.verts.push_back(V(x0, y0, q.u0, q.v0));
      batch.verts.push_back(V(x0, y1, q.u0, q.v1));
      batch.verts.push_back(V(x1, y1, q.u1, q.v1));
      batch.verts.push_back(V(x1, y1, q.u1, q.v1));
      batch.verts.push_back(V(x1, y0, q.u1, q.v0));
      batch.verts.push_back(V(x0, y0, q.u0, q.v0));
    }
  }
}

void TextRenderer::submit(const FontAsset& font, std::string_view text,
                          const TextPlacement& placement,
                          const TextStyle& style, int screenW, int screenH) {
  const TextSpan one{text, style};
  submit(font, std::span<const TextSpan>{&one, 1}, placement, screenW, screenH);
}

}  // namespace engine
