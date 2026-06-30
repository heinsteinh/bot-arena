#pragma once

#include <glm/glm.hpp>

namespace engine {

class Renderer {
 public:
  virtual ~Renderer() = default;

  virtual void beginFrame(int width, int height) = 0;
  virtual void endFrame() = 0;

  virtual void clear(const glm::vec4& color) = 0;
};

}  // namespace engine