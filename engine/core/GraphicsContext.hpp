#pragma once

namespace engine {

class GraphicsContext {
 public:
  virtual ~GraphicsContext() = default;

  virtual void makeCurrent() = 0;
  virtual void setVSync(bool enabled) = 0;
};

}  // namespace engine