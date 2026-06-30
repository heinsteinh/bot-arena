#ifndef GAME_BOTARENAGAME_HPP
#define GAME_BOTARENAGAME_HPP

#include <glm/glm.hpp>

#include "engine/core/Layer.hpp"
#include "engine/renderer/Camera.hpp"

namespace game {

class BotArenaGame final : public engine::Layer {
 public:
  void onAttach() override;
  void onDetach() override;

  void onUpdate(float dt) override;
  void onRender(engine::Renderer& renderer, int width, int height) override;

 private:
  engine::Camera m_camera;

  glm::vec3 m_botPosition{0.0f, 0.0f, 0.0f};
  float m_time = 0.0f;
};

}  // namespace game

#endif  // GAME_BOTARENAGAME_HPP
