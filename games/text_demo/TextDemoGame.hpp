#ifndef GAMES_TEXT_DEMO_TEXTDEMOGAME_HPP
#define GAMES_TEXT_DEMO_TEXTDEMOGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace textdemo {

// Showcases the v0.28 text system: cached font loading, per-string styles,
// screen-space placement. Extended in later slices (SDF crispness, outline,
// glow) as those backends land.
class TextDemoGame final : public engine::Layer {
 public:
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  engine::FontHandle m_font;
  float m_time = 0.0f;
};

}  // namespace textdemo

#endif  // GAMES_TEXT_DEMO_TEXTDEMOGAME_HPP
