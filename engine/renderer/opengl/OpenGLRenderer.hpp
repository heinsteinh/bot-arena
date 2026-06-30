#pragma once

#include "engine/renderer/Renderer.hpp"

namespace engine {

class OpenGLRenderer final : public Renderer {
 public:
  void beginFrame(int width, int height) override;
  void endFrame() override;

  void clear(const glm::vec4& color) override;
};

}  // namespace engine