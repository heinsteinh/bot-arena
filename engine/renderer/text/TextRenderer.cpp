#include "engine/renderer/text/TextRenderer.hpp"

#include "engine/renderer/text/TextLayout.hpp"

namespace engine {

void TextRenderer::clear() {
  m_batches.clear();
  m_index.clear();
}

TextRenderer::Batch& TextRenderer::batchFor(FontBackend backend,
                                            uint32_t atlas) {
  const uint64_t key =
      (static_cast<uint64_t>(backend) << 32) | static_cast<uint64_t>(atlas);
  if (auto it = m_index.find(key); it != m_index.end()) {
    return m_batches[it->second];
  }
  m_index.emplace(key, m_batches.size());
  m_batches.push_back(Batch{atlas, backend, {}});
  return m_batches.back();
}

void TextRenderer::submit(const FontAsset& font, std::string_view text,
                          const TextPlacement& placement,
                          const TextStyle& style, int screenW, int screenH) {
  if (placement.mode != PlacementMode::ScreenSpace) return;  // this slice
  if (screenW <= 0 || screenH <= 0) return;

  const std::vector<TextQuad> quads = layoutText(font.glyphs, text);
  if (quads.empty()) return;

  const uint32_t fill = packColor(style.fillColor);
  const uint32_t outline = packColor(style.outlineColor);
  const uint32_t styleIdx = style.styleIndex;
  const float sw = static_cast<float>(screenW);
  const float sh = static_cast<float>(screenH);
  const auto ndcX = [&](float px) { return px / sw * 2.0f - 1.0f; };
  const auto ndcY = [&](float py) { return 1.0f - py / sh * 2.0f; };

  Batch& batch = batchFor(font.backend, font.atlasRendererID());
  batch.verts.reserve(batch.verts.size() + quads.size() * 6);

  for (const TextQuad& q : quads) {
    // local px -> screen px (placement) -> NDC
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

}  // namespace engine
