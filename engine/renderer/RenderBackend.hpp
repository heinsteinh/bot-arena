#ifndef ENGINE_RENDERER_RENDERBACKEND_HPP
#define ENGINE_RENDERER_RENDERBACKEND_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

#include "engine/core/Arena.hpp"
#include "engine/core/Base.hpp"
#include "engine/renderer/CameraUniforms.hpp"
#include "engine/renderer/LightUniforms.hpp"
#include "engine/renderer/PointLight.hpp"
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
  // Render meshes into the bound MRT G-buffer (albedo/normal/world-pos).
  virtual void executeGeometry(const std::vector<RenderEntry>& entries,
                               const CameraUniforms& camera, Arena& scratch,
                               const ResourceRegistry& registry) = 0;
  // Upload the point-light block (binding 2); count is clamped to 32.
  virtual void setPointLights(int count, const PointLight* lights) = 0;
  // Fullscreen deferred shade: read the G-buffer + shadow map, write HDR.
  virtual void lightingPass(uint32_t gAlbedo, uint32_t gNormal,
                            uint32_t gWorldPos, uint32_t shadowMap) = 0;
  // Render a procedural sky into the 6 faces of an environment cubemap.
  virtual void renderEnvironment(uint32_t cubemap, int size,
                                 const glm::vec3& sunDir) = 0;
  // Remember the environment cubemap the lighting pass samples (unit 4).
  virtual void setEnvironment(uint32_t envCubemap) = 0;
  // Convolve the environment into a diffuse irradiance cubemap.
  virtual void convolveIrradiance(uint32_t env, uint32_t irradianceCube,
                                  int size) = 0;
  // Prefilter the environment for specular IBL into a mipped cubemap.
  virtual void prefilterEnvironment(uint32_t env, uint32_t prefilterCube,
                                    int baseSize, int mipCount) = 0;
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
