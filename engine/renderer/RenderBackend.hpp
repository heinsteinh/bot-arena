#ifndef ENGINE_RENDERER_RENDERBACKEND_HPP
#define ENGINE_RENDERER_RENDERBACKEND_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

#include "engine/core/Arena.hpp"
#include "engine/core/Base.hpp"
#include "engine/renderer/CameraUniforms.hpp"
#include "engine/renderer/LightUniforms.hpp"
#include "engine/renderer/RenderQueue.hpp"

namespace engine {

class ResourceRegistry;
class Framebuffer;

class RenderBackend {
 public:
  virtual ~RenderBackend() = default;

  // Bind `target` (null = default framebuffer + the given viewport) and clear.
  virtual void beginPass(Framebuffer* target, const glm::vec4& clearColor,
                         bool clearDepth, int viewportW, int viewportH) = 0;
  virtual void execute(const std::vector<RenderEntry>& entries,
                       const CameraUniforms& camera, Arena& scratch,
                       const ResourceRegistry& registry) = 0;
  // Render mesh depth from the light's POV into the bound depth target.
  virtual void executeShadow(const std::vector<RenderEntry>& entries,
                             const glm::mat4& lightViewProj, Arena& scratch,
                             const ResourceRegistry& registry) = 0;
  // Upload the light block (binding 1) and remember the shadow map texture.
  virtual void setLight(const LightUniforms& light,
                        uint32_t shadowMapTexture) = 0;
  // Draw a tonemapped quad sampling `sourceColorTexture` into the bound target,
  // covering the NDC rectangle {x0,y0,x1,y1}.
  virtual void blit(uint32_t sourceColorTexture,
                    const glm::vec4& dstRectNDC) = 0;
  virtual void readPixels(int x, int y, int width, int height, void* out) = 0;

  static Scope<RenderBackend> Create();
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERBACKEND_HPP
