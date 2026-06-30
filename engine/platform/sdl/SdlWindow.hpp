#pragma once

#include <SDL3/SDL.h>

#include <string>

#include "engine/core/Window.hpp"

namespace engine {

class SdlWindow final : public Window {
 public:
  SdlWindow(int width, int height, const std::string& title);
  ~SdlWindow() override;

  void pollEvents() override;
  bool shouldClose() const override;
  void swapBuffers() override;

  int width() const override;
  int height() const override;

  void* nativeHandle() const override;

 private:
  SDL_Window* m_window = nullptr;
  bool m_shouldClose = false;
};

}  // namespace engine