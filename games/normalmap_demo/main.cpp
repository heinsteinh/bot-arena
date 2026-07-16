#include <spdlog/spdlog.h>

#include <exception>
#include <memory>

#include "engine/core/Application.hpp"
#include "games/normalmap_demo/NormalMapDemoGame.hpp"

int main() {
  try {
    engine::Application app;
    app.pushLayer(std::make_unique<normalmapdemo::NormalMapDemoGame>());
    app.run();
    return 0;
  } catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }
}
