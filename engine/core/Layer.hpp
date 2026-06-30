#pragma once

#include "engine/renderer/Renderer.hpp"

namespace engine {

class Layer {
 public:
  virtual ~Layer() = default;

  virtual void onAttach() {}
  virtual void onDetach() {}

  virtual void onUpdate(float dt) {}
  virtual void onRender(Renderer& renderer, int width, int height) {}
};

}  // namespace engine