#ifndef GAMES_CSM_DEMO_CSMDEMOGAME_HPP
#define GAMES_CSM_DEMO_CSMDEMOGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace csmdemo {

// A long ground with pillars receding into the distance under a low sun: near
// pillars cast crisp cascade-0 shadows, far pillars stay shadowed by cascades
// 1/2. Run with BOTARENA_CSM=1 to tint the cascade regions.
class CsmDemoGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void ensureResources(engine::Renderer& renderer);

  engine::PerspectiveCamera m_camera;
  float m_time = 0.0f;
  bool m_ready = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_pillarMat = 0;
  engine::FontHandle m_font;
};

}  // namespace csmdemo

#endif  // GAMES_CSM_DEMO_CSMDEMOGAME_HPP
