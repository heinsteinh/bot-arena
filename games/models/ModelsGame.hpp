#ifndef GAMES_MODELS_MODELSGAME_HPP
#define GAMES_MODELS_MODELSGAME_HPP

#include <string>
#include <vector>

#include "engine/assets/ModelLoader.hpp"
#include "engine/core/Layer.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/RenderCommand.hpp"

namespace models {

class ModelsGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;
  void onImGuiRender() override;

 private:
  engine::OrbitCameraController m_camera;
  bool m_resourcesReady = false;
  engine::MaterialHandle m_modelMat = 0;
  engine::MaterialHandle m_groundMat = 0;

  struct Entry {
    std::string name;
    std::string path;
    engine::Model model;
  };
  std::vector<Entry> m_entries;
  int m_selected = 0;
  float m_angle = 0.0f;
  bool m_autoRotate = true;
};

}  // namespace models

#endif  // GAMES_MODELS_MODELSGAME_HPP
