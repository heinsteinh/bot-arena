#ifndef ENGINE_RENDERER_RENDERBACKEND_HPP
#define ENGINE_RENDERER_RENDERBACKEND_HPP

#include <glm/glm.hpp>
#include <vector>

#include "engine/core/Arena.hpp"
#include "engine/core/Base.hpp"
#include "engine/renderer/RenderQueue.hpp"

namespace engine {

class ResourceRegistry;

class RenderBackend {
 public:
  virtual ~RenderBackend() = default;

  virtual void beginFrame(int width, int height) = 0;
  virtual void execute(const std::vector<RenderEntry>& entries,
                       const glm::mat4& viewProjection, Arena& scratch,
                       const ResourceRegistry& registry) = 0;
  virtual void endFrame() = 0;
  virtual void readPixels(int x, int y, int width, int height, void* out) = 0;

  static Scope<RenderBackend> Create();
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERBACKEND_HPP
