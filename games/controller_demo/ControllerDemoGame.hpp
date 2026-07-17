#ifndef GAMES_CONTROLLER_DEMO_CONTROLLERDEMOGAME_HPP
#define GAMES_CONTROLLER_DEMO_CONTROLLERDEMOGAME_HPP

#include <vector>

#include "engine/core/Layer.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"
#include "engine/scene/Scene.hpp"
#include "engine/scene/SceneObject.hpp"

namespace controllerdemo {

// Cycles a scene camera through the four controller components (Fly/Orbit/
// Follow/2D). Space cycles live; BOTARENA_CTRL=0..3 selects for screenshots.
class ControllerDemoGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void ensureResources(engine::Renderer& renderer);
  void setController(int index);

  engine::Scene m_scene;
  std::vector<engine::SceneObject> m_visuals;
  engine::SceneObject m_camera;
  engine::SceneObject m_followTarget;
  int m_controller = 0;
  bool m_screenshot = false;
  bool m_ready = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_cubeMat = 0;
  engine::FontHandle m_font;
};

}  // namespace controllerdemo

#endif  // GAMES_CONTROLLER_DEMO_CONTROLLERDEMOGAME_HPP
