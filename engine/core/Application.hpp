#pragma once

#include <memory>

#include "engine/core/GraphicsContext.hpp"
#include "engine/core/Window.hpp"
#include "engine/renderer/Renderer.hpp"

namespace engine {

class Application {
 public:
  Application();
  ~Application();

  void run();

 private:
  std::unique_ptr<Window> m_window;
  std::unique_ptr<GraphicsContext> m_context;
  std::unique_ptr<Renderer> m_renderer;
};

}  // namespace engine