#ifndef ENGINE_CORE_WINDOW_HPP
#define ENGINE_CORE_WINDOW_HPP

namespace engine {

class Window {
 public:
  virtual ~Window() = default;

  virtual void pollEvents() = 0;
  virtual bool shouldClose() const = 0;
  virtual void swapBuffers() = 0;

  virtual int width() const = 0;
  virtual int height() const = 0;

  virtual void* nativeHandle() const = 0;
};

}  // namespace engine

#endif  // ENGINE_CORE_WINDOW_HPP
