#ifndef ENGINE_PLATFORM_SDL_SDLWINDOW_HPP
#define ENGINE_PLATFORM_SDL_SDLWINDOW_HPP

#include <SDL3/SDL.h>

#include <functional>
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
  void setEventCallback(std::function<bool(void*)> callback) override {
    m_eventCallback = std::move(callback);
  }

 private:
  SDL_Window* m_window = nullptr;
  bool m_shouldClose = false;
  std::function<bool(void*)> m_eventCallback;
};

}  // namespace engine

#endif  // ENGINE_PLATFORM_SDL_SDLWINDOW_HPP
