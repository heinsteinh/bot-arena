#ifndef GAMES_NORMALMAP_DEMO_NORMALMAPDEMOGAME_HPP
#define GAMES_NORMALMAP_DEMO_NORMALMAPDEMOGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace normalmapdemo {

// Two brick walls side by side under a moving light: the left uses only an
// albedo map (flat), the right adds a tangent-space normal map (relief). Proves
// the g-buffer normal is perturbed and picked up by the deferred lighting.
// BOTARENA_LIGHT=0|1 freezes the light at one of two raking angles for
// headless before/after screenshots.
class NormalMapDemoGame final : public engine::Layer {
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
  engine::MaterialHandle m_flatMat = 0;
  engine::MaterialHandle m_mappedMat = 0;
  engine::FontHandle m_font;
};

}  // namespace normalmapdemo

#endif  // GAMES_NORMALMAP_DEMO_NORMALMAPDEMOGAME_HPP
