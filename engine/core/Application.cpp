#include "engine/core/Application.hpp"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <glm/glm.hpp>

#include "engine/core/Input.hpp"
#include "engine/core/Time.hpp"
#include "engine/platform/sdl/SdlOpenGLContext.hpp"
#include "engine/platform/sdl/SdlWindow.hpp"
#include "engine/renderer/opengl/OpenGLRenderer.hpp"

namespace engine {

Application::Application() {
  auto sdlWindow =
      std::make_unique<SdlWindow>(1280, 720, "Bot Arena - AI Sandbox Engine");

  auto* rawSdlWindow = sdlWindow.get();

  m_window = std::move(sdlWindow);
  m_context = std::make_unique<SdlOpenGLContext>(*rawSdlWindow);
  m_renderer = std::make_unique<OpenGLRenderer>();

  spdlog::info("Application initialized");
}

Application::~Application() {
  for (auto it = m_layers.rbegin(); it != m_layers.rend(); ++it) {
    (*it)->onDetach();
  }
}

void Application::pushLayer(std::unique_ptr<Layer> layer) {
  layer->onAttach();
  m_layers.push_back(std::move(layer));
}

void Application::run() {
  // When set, render a single frame, save it to the given path, then exit.
  const char* screenshotPath = std::getenv("BOTARENA_SCREENSHOT");

  while (!m_window->shouldClose()) {
    Time::update();
    Input::beginFrame();

    m_window->pollEvents();

    const float dt = Time::deltaTime();

    for (auto& layer : m_layers) {
      layer->onUpdate(dt);
    }

    m_renderer->beginFrame(m_window->width(), m_window->height());
    m_renderer->clear(glm::vec4{0.08f, 0.09f, 0.11f, 1.0f});

    for (auto& layer : m_layers) {
      layer->onRender(*m_renderer, m_window->width(), m_window->height());
    }

    m_renderer->endFrame();

    // Capture the back buffer before it is swapped out, then stop.
    if (screenshotPath) {
      m_renderer->saveScreenshot(screenshotPath, m_window->width(),
                                 m_window->height());
      break;
    }

    m_window->swapBuffers();
  }
}

}  // namespace engine