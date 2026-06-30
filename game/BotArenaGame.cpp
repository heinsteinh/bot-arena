#include "game/BotArenaGame.hpp"

#include <spdlog/spdlog.h>

#include <cmath>

namespace game {

void BotArenaGame::onAttach() { spdlog::info("BotArenaGame attached"); }

void BotArenaGame::onDetach() { spdlog::info("BotArenaGame detached"); }

void BotArenaGame::onUpdate(float dt) {
  m_time += dt;

  m_botPosition.x = std::sin(m_time) * 2.0f;
  m_botPosition.y = 0.5f;
  m_botPosition.z = std::cos(m_time) * 2.0f;
}

void BotArenaGame::onRender(engine::Renderer& renderer) {
  renderer.drawGrid(10.0f, 1.0f, {0.25f, 0.25f, 0.25f, 1.0f});

  renderer.drawCube({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f},
                    {0.7f, 0.7f, 0.7f, 1.0f});
  renderer.drawCube({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f},
                    {0.7f, 0.7f, 0.7f, 1.0f});
  renderer.drawCube({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f},
                    {0.7f, 0.7f, 0.7f, 1.0f});
  renderer.drawCube({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f},
                    {0.7f, 0.7f, 0.7f, 1.0f});

  renderer.drawCube({2.0f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f},
                    {0.2f, 0.6f, 1.0f, 1.0f});
  renderer.drawCube({-2.0f, 0.5f, -2.0f}, {1.5f, 1.0f, 1.5f},
                    {0.2f, 0.6f, 1.0f, 1.0f});

  renderer.drawCube(m_botPosition, {0.5f, 1.0f, 0.5f},
                    {1.0f, 0.2f, 0.2f, 1.0f});

  renderer.drawLine(m_botPosition, {0.0f, 0.5f, 0.0f},
                    {1.0f, 1.0f, 0.0f, 1.0f});
}

}  // namespace game