#include "engine/core/Application.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <spdlog/spdlog.h>

#include <cstdlib>
#include <glm/glm.hpp>
#include <string>

#include "engine/core/DebugOverlay.hpp"
#include "engine/core/Input.hpp"
#include "engine/core/JobSystem.hpp"
#include "engine/core/Time.hpp"
#include "engine/platform/sdl/SdlOpenGLContext.hpp"
#include "engine/platform/sdl/SdlWindow.hpp"
#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextStyle.hpp"

namespace engine {

Application::Application() {
  auto sdlWindow =
      std::make_unique<SdlWindow>(1280, 720, "Bot Arena - AI Sandbox Engine");

  auto* rawSdlWindow = sdlWindow.get();

  m_window = std::move(sdlWindow);
  m_context = std::make_unique<SdlOpenGLContext>(*rawSdlWindow);
  m_jobs = std::make_unique<JobSystem>();
  m_renderer = std::make_unique<Renderer>(*m_jobs);

  m_imguiLayer = std::make_unique<ImGuiLayer>(m_window->nativeHandle(),
                                              m_context->nativeContext());
  m_imguiLayer->onAttach();
  m_window->setEventCallback([](void* e) -> bool {
    SDL_Event* ev = static_cast<SDL_Event*>(e);
    ImGui_ImplSDL3_ProcessEvent(ev);
    const ImGuiIO& io = ImGui::GetIO();
    switch (ev->type) {
      case SDL_EVENT_MOUSE_MOTION:
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      case SDL_EVENT_MOUSE_WHEEL:
        return io.WantCaptureMouse;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
      case SDL_EVENT_TEXT_INPUT:
        return io.WantCaptureKeyboard;
      default:
        return false;
    }
  });

  spdlog::info("Application initialized");
}

Application::~Application() {
  if (m_imguiLayer) m_imguiLayer->onDetach();
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

    m_jobs->resetStats();
    if (Input::wasKeyPressed(Key::F1)) m_showDebug = !m_showDebug;

    const float dt = Time::deltaTime();

    for (auto& layer : m_layers) {
      layer->onUpdate(dt);
    }

    m_renderer->beginFrame(m_window->width(), m_window->height());

    for (auto& layer : m_layers) {
      layer->onRender(*m_renderer, m_window->width(), m_window->height());
    }

    const float inst = dt > 1e-5f ? 1.0f / dt : 0.0f;
    m_fps = m_fps <= 0.0f ? inst : m_fps + (inst - m_fps) * 0.1f;
    if (m_showDebug) {
      if (!m_debugFont) {
        engine::FontDesc desc;
        desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf";
        desc.pixelSize = 32;
        desc.backend = engine::FontBackend::Bitmap;
        m_debugFont = m_renderer->fonts().load(desc);
      }
      if (m_debugFont) {
        DebugStats ds;
        ds.fps = static_cast<int>(m_fps + 0.5f);
        ds.frameMs = m_fps > 0.0f ? 1000.0f / m_fps : 0.0f;
        ds.width = m_window->width();
        ds.height = m_window->height();
        const Renderer::RenderStats rs = m_renderer->stats();
        ds.cameraPos = rs.cameraPos;
        ds.cameraFwd = rs.cameraFwd;
        ds.drawCount = rs.drawCount;
        ds.pointLights = rs.pointLights;
        ds.laneCount = rs.laneCount;
        const JobSystem::Stats& js = m_jobs->stats();
        ds.jobDispatches = js.dispatches;
        ds.jobBatches = js.batches;
        ds.jobItems = js.items;
        ds.laneBatches = js.laneBatches;
        const std::vector<std::string> lines = formatDebugLines(ds);
        for (size_t i = 0; i < lines.size(); ++i) {
          engine::TextPlacement p;
          p.pos = {8.0f, 20.0f + static_cast<float>(i) * 22.0f};
          p.scale = 0.7f;
          engine::TextStyle s;  // white fill (default)
          m_renderer->drawText(m_debugFont, lines[i], p, s);
        }
      }
    }

    m_renderer->endFrame();

    m_imguiLayer->begin();
    for (auto& layer : m_layers) {
      layer->onImGuiRender();
    }
    m_imguiLayer->end();

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