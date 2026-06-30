#include "engine/platform/sdl/SdlOpenGLContext.hpp"

#include <glad/glad.h>

#include <stdexcept>

namespace engine {

SdlOpenGLContext::SdlOpenGLContext(SdlWindow& window) {
  m_window = static_cast<SDL_Window*>(window.nativeHandle());

  m_context = SDL_GL_CreateContext(m_window);

  if (!m_context) {
    throw std::runtime_error("Failed to create SDL OpenGL context");
  }

  makeCurrent();

  if (!gladLoadGLLoader(
          reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
    throw std::runtime_error("Failed to load OpenGL functions with GLAD");
  }

  setVSync(true);
}

SdlOpenGLContext::~SdlOpenGLContext() {
  if (m_context) {
    SDL_GL_DestroyContext(m_context);
  }
}

void SdlOpenGLContext::makeCurrent() {
  SDL_GL_MakeCurrent(m_window, m_context);
}

void SdlOpenGLContext::setVSync(bool enabled) {
  SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}

}  // namespace engine