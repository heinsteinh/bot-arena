#include "engine/renderer/text/Font.hpp"

#include <ft2build.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <vector>
#include FT_FREETYPE_H

namespace engine {

Ref<Font> Font::Load(const std::string& ttfPath, uint32_t pixelSize) {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    spdlog::error("FreeType init failed");
    return nullptr;
  }
  FT_Face face;
  if (FT_New_Face(ft, ttfPath.c_str(), 0, &face)) {
    spdlog::error("Failed to load font: {}", ttfPath);
    FT_Done_FreeType(ft);
    return nullptr;
  }
  FT_Set_Pixel_Sizes(face, 0, pixelSize);

  const int pad = 1;
  const int atlasW = 512;

  struct Pending {
    unsigned char c;
    int x, y, w, h, bx, by;
    float adv;
    std::vector<unsigned char> bmp;
  };
  std::vector<Pending> pend;
  int penX = pad, penY = pad, rowH = 0;
  for (unsigned char c = 32; c <= 126; ++c) {
    if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
    FT_GlyphSlot g = face->glyph;
    const int w = static_cast<int>(g->bitmap.width);
    const int h = static_cast<int>(g->bitmap.rows);
    if (penX + w + pad > atlasW) {
      penX = pad;
      penY += rowH + pad;
      rowH = 0;
    }
    Pending p;
    p.c = c;
    p.x = penX;
    p.y = penY;
    p.w = w;
    p.h = h;
    p.bx = g->bitmap_left;
    p.by = g->bitmap_top;
    p.adv = static_cast<float>(g->advance.x >> 6);
    p.bmp.assign(g->bitmap.buffer, g->bitmap.buffer + (w * h));
    pend.push_back(std::move(p));
    penX += w + pad;
    rowH = std::max(rowH, h);
  }
  const int atlasH = penY + rowH + pad;

  std::vector<unsigned char> atlas(static_cast<size_t>(atlasW) * atlasH, 0);
  GlyphMap glyphs{};
  for (const Pending& p : pend) {
    for (int row = 0; row < p.h; ++row) {
      for (int col = 0; col < p.w; ++col) {
        atlas[(p.y + row) * atlasW + (p.x + col)] = p.bmp[row * p.w + col];
      }
    }
    Glyph gl;
    gl.size = {static_cast<float>(p.w), static_cast<float>(p.h)};
    gl.bearing = {static_cast<float>(p.bx), static_cast<float>(p.by)};
    gl.advance = p.adv;
    gl.uvMin = {static_cast<float>(p.x) / atlasW,
                static_cast<float>(p.y) / atlasH};
    gl.uvMax = {static_cast<float>(p.x + p.w) / atlasW,
                static_cast<float>(p.y + p.h) / atlasH};
    glyphs[p.c] = gl;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  Ref<Texture2D> tex = Texture2D::Create(static_cast<uint32_t>(atlasW),
                                         static_cast<uint32_t>(atlasH));
  tex->setData(atlas.data(), static_cast<uint32_t>(atlas.size()));
  return CreateRef<Font>(glyphs, std::move(tex));
}

}  // namespace engine
