#pragma once

#include <SDL3/SDL.h>

#include "engine/core/GraphicsContext.hpp"
#include "engine/platform/sdl/SdlWindow.hpp"

namespace engine {

class SdlOpenGLContext final : public GraphicsContext {
 public:
  explicit SdlOpenGLContext(SdlWindow& window);
  ~SdlOpenGLContext() override;

  void makeCurrent() override;
  void setVSync(bool enabled) override;

 private:
  SDL_Window* m_window = nullptr;
  SDL_GLContext m_context = nullptr;
};

}  // namespace engine