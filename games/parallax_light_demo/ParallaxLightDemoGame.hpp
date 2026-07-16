#ifndef GAMES_PARALLAX_LIGHT_DEMO_PARALLAXLIGHTDEMOGAME_HPP
#define GAMES_PARALLAX_LIGHT_DEMO_PARALLAXLIGHTDEMOGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace parallaxlightdemo {

// A parallax brick floor under a point light orbiting low: the mortar
// self-shadows sweep to follow the light, proving the parallax self-shadow is
// per-point-light (not the directional one). BOTARENA_LIGHT=0|1|2 freezes the
// orbiting light for headless screenshots.
class ParallaxLightDemoGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void ensureResources(engine::Renderer& renderer);

  engine::PerspectiveCamera m_camera;
  float m_time = 0.0f;
  bool m_screenshot = false;
  int m_lightPreset = 0;

  bool m_ready = false;
  engine::MaterialHandle m_floorMat = 0;
  engine::FontHandle m_font;
};

}  // namespace parallaxlightdemo

#endif  // GAMES_PARALLAX_LIGHT_DEMO_PARALLAXLIGHTDEMOGAME_HPP
