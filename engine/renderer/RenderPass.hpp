#ifndef ENGINE_RENDERER_RENDERPASS_HPP
#define ENGINE_RENDERER_RENDERPASS_HPP

#include <glm/glm.hpp>

#include "engine/core/Base.hpp"
#include "engine/renderer/Framebuffer.hpp"

namespace engine {

// A render target and how to clear it. Backend-agnostic data sequenced by the
// Renderer. A null target means the default framebuffer.
struct RenderPass {
  Ref<Framebuffer> target;
  glm::vec4 clearColor{0.08f, 0.09f, 0.11f, 1.0f};
  bool clearDepth = true;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERPASS_HPP
