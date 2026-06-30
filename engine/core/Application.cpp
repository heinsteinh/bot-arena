#include "engine/core/Application.hpp"

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>

#include "engine/platform/sdl/SdlOpenGLContext.hpp"
#include "engine/platform/sdl/SdlWindow.hpp"
#include "engine/renderer/opengl/OpenGLRenderer.hpp"

namespace engine {

Application::Application() {
  auto sdlWindow =
      std::make_unique<SdlWindow>(1280, 720, "Bot Arena - Engine Abstraction");

  auto* rawSdlWindow = sdlWindow.get();

  m_window = std::move(sdlWindow);
  m_context = std::make_unique<SdlOpenGLContext>(*rawSdlWindow);
  m_renderer = std::make_unique<OpenGLRenderer>();

  spdlog::info("Application initialized");
}

Application::~Application() = default;

void Application::run() {
  while (!m_window->shouldClose()) {
    m_window->pollEvents();

    m_renderer->beginFrame(m_window->width(), m_window->height());
    m_renderer->clear(glm::vec4{0.08f, 0.09f, 0.11f, 1.0f});
    m_renderer->endFrame();

    m_window->swapBuffers();
  }
}

}  // namespace engine