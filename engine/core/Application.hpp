#ifndef ENGINE_CORE_APPLICATION_HPP
#define ENGINE_CORE_APPLICATION_HPP

#include <memory>
#include <vector>

#include "engine/core/GraphicsContext.hpp"
#include "engine/core/Layer.hpp"
#include "engine/core/Window.hpp"
#include "engine/renderer/Renderer.hpp"

namespace engine {

class Application {
 public:
  Application();
  ~Application();

  void pushLayer(std::unique_ptr<Layer> layer);
  void run();

 private:
  std::unique_ptr<Window> m_window;
  std::unique_ptr<GraphicsContext> m_context;
  std::unique_ptr<Renderer> m_renderer;

  std::vector<std::unique_ptr<Layer>> m_layers;
};
}  // namespace engine

#endif  // ENGINE_CORE_APPLICATION_HPP
