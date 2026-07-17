#ifndef GAMES_MODELS_MODELSGAME_HPP
#define GAMES_MODELS_MODELSGAME_HPP

#include <string>
#include <vector>

#include "engine/core/Layer.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/scene/Scene.hpp"
#include "engine/scene/SceneObject.hpp"

namespace models {

class ModelsGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;
  void onImGuiRender() override;

 private:
  engine::Scene m_scene;
  engine::SceneObject m_camera;
  engine::SceneObject m_ground;
  engine::SceneObject m_model;  // the displayed model entity
  bool m_resourcesReady = false;
  engine::MaterialHandle m_groundMat = 0;

  struct Entry {
    std::string name;
    std::string path;
    engine::ModelHandle handle = 0;
    bool valid = false;
  };
  std::vector<Entry> m_entries;
  int m_selected = 0;
  float m_angle = 0.0f;
  bool m_autoRotate = true;
};

}  // namespace models

#endif  // GAMES_MODELS_MODELSGAME_HPP
