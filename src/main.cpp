#include <spdlog/spdlog.h>

#include <exception>

#include "engine/core/Application.hpp"

int main() {
  try {
    engine::Application app;
    app.run();
    return 0;
  } catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    return 1;
  }
}