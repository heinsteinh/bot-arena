#include "engine/platform/sdl/SdlWindow.hpp"

#include <stdexcept>

#include "engine/core/Input.hpp"

namespace engine {

namespace {

Key mapKey(SDL_Keycode key) {
  switch (key) {
    case SDLK_ESCAPE:
      return Key::Escape;
    case SDLK_W:
      return Key::W;
    case SDLK_A:
      return Key::A;
    case SDLK_S:
      return Key::S;
    case SDLK_D:
      return Key::D;
    case SDLK_Q:
      return Key::Q;
    case SDLK_E:
      return Key::E;
    case SDLK_SPACE:
      return Key::Space;
    case SDLK_LSHIFT:
      return Key::LeftShift;
    default:
      return Key::Unknown;
  }
}

MouseButton mapMouseButton(unsigned char button) {
  switch (button) {
    case SDL_BUTTON_LEFT:
      return MouseButton::Left;
    case SDL_BUTTON_RIGHT:
      return MouseButton::Right;
    case SDL_BUTTON_MIDDLE:
      return MouseButton::Middle;
    default:
      return MouseButton::Left;
  }
}

}  // namespace

SdlWindow::SdlWindow(int width, int height, const std::string& title) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error("Failed to initialize SDL3");
  }

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  m_window = SDL_CreateWindow(title.c_str(), width, height,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (!m_window) {
    throw std::runtime_error("Failed to create SDL3 window");
  }
}

SdlWindow::~SdlWindow() {
  if (m_window) {
    SDL_DestroyWindow(m_window);
  }

  SDL_Quit();
}

void SdlWindow::pollEvents() {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_QUIT) {
      m_shouldClose = true;
    }

    if (event.type == SDL_EVENT_KEY_DOWN) {
      const Key key = mapKey(event.key.key);

      if (key == Key::Escape) {
        m_shouldClose = true;
      }

      if (key != Key::Unknown) {
        Input::setKey(key, true);
      }
    }

    if (event.type == SDL_EVENT_KEY_UP) {
      const Key key = mapKey(event.key.key);

      if (key != Key::Unknown) {
        Input::setKey(key, false);
      }
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
      Input::setMouseButton(mapMouseButton(event.button.button), true);
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
      Input::setMouseButton(mapMouseButton(event.button.button), false);
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION) {
      Input::setMousePosition(event.motion.x, event.motion.y);
      Input::setMouseDelta(event.motion.xrel, event.motion.yrel);
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
      Input::setScrollDelta(event.wheel.x, event.wheel.y);
    }
  }
}

bool SdlWindow::shouldClose() const { return m_shouldClose; }

void SdlWindow::swapBuffers() { SDL_GL_SwapWindow(m_window); }

int SdlWindow::width() const {
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(m_window, &w, &h);
  return w;
}

int SdlWindow::height() const {
  int w = 0;
  int h = 0;
  SDL_GetWindowSize(m_window, &w, &h);
  return h;
}

void* SdlWindow::nativeHandle() const { return m_window; }

}  // namespace engine