#ifndef ENGINE_RENDERER_TEXT_FONTDESC_HPP
#define ENGINE_RENDERER_TEXT_FONTDESC_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace engine {

enum class FontBackend { Bitmap, SDF, MSDF };
enum class SourcePolicy { Offline, Runtime, Auto };
enum class RenderTarget { Screen, World };  // reserved

struct GlyphRange {
  char32_t first = 32;
  char32_t last = 126;
  bool operator==(const GlyphRange& o) const {
    return first == o.first && last == o.last;
  }
};

// Identifies a font to load; the normalized value is the FontManager cache key.
struct FontDesc {
  std::string family;  // path or family name
  uint32_t pixelSize = 32;
  FontBackend backend = FontBackend::Bitmap;
  SourcePolicy source = SourcePolicy::Offline;
  GlyphRange glyphRange{};
  float dpiScale = 1.0f;                             // reserved
  RenderTarget renderTarget = RenderTarget::Screen;  // reserved

  bool operator==(const FontDesc& o) const {
    return family == o.family && pixelSize == o.pixelSize &&
           backend == o.backend && source == o.source &&
           glyphRange == o.glyphRange && dpiScale == o.dpiScale &&
           renderTarget == o.renderTarget;
  }
};

struct FontDescHash {
  std::size_t operator()(const FontDesc& d) const {
    std::size_t h = std::hash<std::string>{}(d.family);
    const auto mix = [&h](std::size_t v) {
      h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    };
    mix(d.pixelSize);
    mix(static_cast<std::size_t>(d.backend));
    mix(static_cast<std::size_t>(d.source));
    mix(d.glyphRange.first);
    mix(d.glyphRange.last);
    mix(std::hash<float>{}(d.dpiScale));
    mix(static_cast<std::size_t>(d.renderTarget));
    return h;
  }
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_FONTDESC_HPP
