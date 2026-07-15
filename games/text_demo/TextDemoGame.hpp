#ifndef GAMES_TEXT_DEMO_TEXTDEMOGAME_HPP
#define GAMES_TEXT_DEMO_TEXTDEMOGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace textdemo {

// Showcases the v0.28 text system: cached loading of multiple fonts, per-string
// styles, and screen-space placement. Outline / stroke / shadow / cartoon are
// composed here at the app level by layering draws (the classic bitmap
// technique); crisp single-pass SDF/MSDF versions land in later slices.
class TextDemoGame final : public engine::Layer {
 public:
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void loadFonts(engine::Renderer& renderer);

  engine::FontHandle m_sans;     // DejaVu Sans (clean UI face)
  engine::FontHandle m_script;   // Lobster (decorative script)
  engine::FontHandle m_display;  // Bauhaus 93 (heavy display face)
  float m_time = 0.0f;
};

}  // namespace textdemo

#endif  // GAMES_TEXT_DEMO_TEXTDEMOGAME_HPP
