#ifndef ENGINE_CORE_GRAPHICSCONTEXT_HPP
#define ENGINE_CORE_GRAPHICSCONTEXT_HPP

namespace engine {

class GraphicsContext {
 public:
  virtual ~GraphicsContext() = default;

  virtual void makeCurrent() = 0;
  virtual void setVSync(bool enabled) = 0;
};

}  // namespace engine

#endif  // ENGINE_CORE_GRAPHICSCONTEXT_HPP
