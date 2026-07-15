#include <spdlog/spdlog.h>

#include <exception>
#include <memory>

#include "engine/core/Application.hpp"
#include "games/billboard_demo/BillboardDemoGame.hpp"

int main() {
  try {
    engine::Application app;
    app.pushLayer(std::make_unique<billboarddemo::BillboardDemoGame>());
    app.run();
    return 0;
  } catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }
}
