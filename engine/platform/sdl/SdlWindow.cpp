#include "engine/platform/sdl/SdlWindow.hpp"

#include <stdexcept>

namespace engine {

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

    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
      m_shouldClose = true;
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