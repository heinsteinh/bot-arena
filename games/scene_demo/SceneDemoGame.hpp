#ifndef GAMES_SCENE_DEMO_SCENEDEMOGAME_HPP
#define GAMES_SCENE_DEMO_SCENEDEMOGAME_HPP

#include <vector>

#include "engine/core/Layer.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"
#include "engine/scene/Scene.hpp"
#include "engine/scene/SceneObject.hpp"

namespace scenedemo {

// Proves the scene-owned primary camera drives the renderer and that each
// object's TransformComponent is the single source of truth for its placement.
class SceneDemoGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void ensureResources(engine::Renderer& renderer);

  engine::Scene m_scene;
  std::vector<engine::SceneObject> m_visuals;
  engine::SceneObject m_camera;
  bool m_ready = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_cubeMat = 0;
  engine::FontHandle m_font;
};

}  // namespace scenedemo

#endif  // GAMES_SCENE_DEMO_SCENEDEMOGAME_HPP
