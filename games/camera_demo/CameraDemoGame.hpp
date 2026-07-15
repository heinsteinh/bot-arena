#ifndef GAMES_CAMERA_DEMO_CAMERADEMOGAME_HPP
#define GAMES_CAMERA_DEMO_CAMERADEMOGAME_HPP

#include "engine/core/Layer.hpp"
#include "engine/renderer/Camera.hpp"
#include "engine/renderer/FlyCameraController.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/OrthographicCamera.hpp"
#include "engine/renderer/PerspectiveCamera.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace camerademo {

// Views a shared 3D scene through four cameras (orbit / fly / top-down ortho /
// fixed front) with an SDF-effects HUD, demonstrating that screen-space text
// renders over any camera projection. Space cycles the view; BOTARENA_VIEW=0..3
// sets the initial one for headless screenshots.
class CameraDemoGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  const engine::Camera& activeCamera(float aspect);
  void ensureResources(engine::Renderer& renderer);

  engine::OrbitCameraController m_orbit;
  engine::FlyCameraController m_fly;
  engine::PerspectiveCamera m_front;
  engine::OrthographicCamera m_top;
  int m_view = 0;
  float m_time = 0.0f;

  bool m_resourcesReady = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_cubeMats[4] = {0, 0, 0, 0};
  engine::FontHandle m_font;
};

}  // namespace camerademo

#endif  // GAMES_CAMERA_DEMO_CAMERADEMOGAME_HPP
