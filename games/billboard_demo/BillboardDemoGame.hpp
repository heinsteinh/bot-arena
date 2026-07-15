#ifndef GAMES_BILLBOARD_DEMO_BILLBOARDDEMOGAME_HPP
#define GAMES_BILLBOARD_DEMO_BILLBOARDDEMOGAME_HPP

#include <string>
#include <vector>

#include "engine/core/Layer.hpp"
#include "engine/renderer/OrbitCameraController.hpp"
#include "engine/renderer/RenderCommand.hpp"
#include "engine/renderer/text/FontAsset.hpp"

namespace billboarddemo {

struct DamageNumber {
  glm::vec3 worldPos{0.0f};
  std::string text;  // owned (not a transient string_view)
  glm::vec4 color{1.0f};
  float scaleMul = 1.0f;  // crit numbers are bigger
  float age = 0.0f;
};

class BillboardDemoGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  void ensureResources(engine::Renderer& renderer);

  engine::OrbitCameraController m_camera;
  std::vector<glm::vec3> m_enemies;
  std::vector<DamageNumber> m_numbers;
  float m_time = 0.0f;
  bool m_screenshot = false;

  bool m_ready = false;
  engine::MaterialHandle m_groundMat = 0;
  engine::MaterialHandle m_enemyMat = 0;
  engine::FontHandle m_font;
};

}  // namespace billboarddemo

#endif  // GAMES_BILLBOARD_DEMO_BILLBOARDDEMOGAME_HPP
