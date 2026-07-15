#include "engine/renderer/text/SdfFreeTypeSource.hpp"

#include <ft2build.h>
#include <spdlog/spdlog.h>

#include <vector>

#include "engine/renderer/text/ShelfPacker.hpp"
#include FT_FREETYPE_H
#include FT_MODULE_H  // FT_Property_Set

namespace engine {

bool SdfFreeTypeSource::bake(const FontDesc& desc, BakedFont& out) {
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    spdlog::error("FreeType init failed");
    return false;
  }
  // Distance range (px) baked around each edge; 0.5 sample == glyph edge.
  const FT_Int spread = 8;
  FT_Property_Set(ft, "sdf", "spread", &spread);

  FT_Face face;
  if (FT_New_Face(ft, desc.family.c_str(), 0, &face)) {
    spdlog::error("Failed to load font: {}", desc.family);
    FT_Done_FreeType(ft);
    return false;
  }
  FT_Set_Pixel_Sizes(face, 0, desc.pixelSize);

  const int atlasW = 512;
  ShelfPacker packer(atlasW, /*pad=*/1);

  struct Pending {
    char32_t cp;
    int x, y, w, h, bx, by;
    float adv;
    std::vector<unsigned char> bmp;
  };
  std::vector<Pending> pend;
  for (char32_t c = desc.glyphRange.first; c <= desc.glyphRange.last; ++c) {
    if (FT_Load_Char(face, c, FT_LOAD_DEFAULT)) continue;  // load outline
    FT_GlyphSlot g = face->glyph;
    // Render an SDF only when there is an outline; blanks (space) keep advance.
    if (g->format == FT_GLYPH_FORMAT_OUTLINE && g->outline.n_points > 0) {
      if (FT_Render_Glyph(g, FT_RENDER_MODE_SDF)) {
        // Render failed for this glyph: treat as a blank, keep advance only.
        g->bitmap.width = 0;
        g->bitmap.rows = 0;
      }
    }
    const int w = static_cast<int>(g->bitmap.width);
    const int h = static_cast<int>(g->bitmap.rows);
    const glm::ivec2 at = packer.place(w, h);
    Pending p;
    p.cp = c;
    p.x = at.x;
    p.y = at.y;
    p.w = w;
    p.h = h;
    p.bx = g->bitmap_left;  // adjusted by FreeType for the SDF spread padding
    p.by = g->bitmap_top;
    p.adv = static_cast<float>(g->advance.x >> 6);
    if (w > 0 && h > 0) {
      p.bmp.assign(g->bitmap.buffer, g->bitmap.buffer + (w * h));
    }
    pend.push_back(std::move(p));
  }
  const int atlasH = packer.height();

  out.atlasWidth = atlasW;
  out.atlasHeight = atlasH;
  out.pxRange = static_cast<float>(spread);
  out.atlasPixels.assign(static_cast<size_t>(atlasW) * atlasH, 0);
  for (const Pending& p : pend) {
    for (int row = 0; row < p.h; ++row) {
      for (int col = 0; col < p.w; ++col) {
        out.atlasPixels[(p.y + row) * atlasW + (p.x + col)] =
            p.bmp[row * p.w + col];
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
    out.glyphs[p.cp] = gl;
  }

  out.metrics.pixelSize = static_cast<float>(desc.pixelSize);
  out.metrics.ascent = static_cast<float>(face->size->metrics.ascender >> 6);
  out.metrics.descent = static_cast<float>(face->size->metrics.descender >> 6);
  out.metrics.lineGap = static_cast<float>(face->size->metrics.height >> 6);

  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  return true;
}

}  // namespace engine
