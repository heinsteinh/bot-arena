#ifndef ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP

#include "engine/core/Base.hpp"
#include "engine/renderer/RenderBackend.hpp"
#include "engine/renderer/UniformBuffer.hpp"

namespace engine {

class Framebuffer;

class OpenGLBackend final : public RenderBackend {
 public:
  OpenGLBackend();
  ~OpenGLBackend() override;

  void beginPass(Framebuffer* target, const glm::vec4& clearColor,
                 bool clearDepth, int viewportW, int viewportH) override;
  void executeGeometry(const std::vector<RenderEntry>& entries,
                       const CameraUniforms& camera, Arena& scratch,
                       const ResourceRegistry& registry) override;
  void setPointLights(int count, const PointLight* lights) override;
  void lightingPass(uint32_t gAlbedo, uint32_t gNormal, uint32_t gWorldPos,
                    uint32_t shadowMap) override;
  void executeShadow(const std::vector<RenderEntry>& entries,
                     const glm::mat4& lightViewProj, Arena& scratch,
                     const ResourceRegistry& registry) override;
  void setLight(const LightUniforms& light, uint32_t shadowMapTexture) override;
  void blit(uint32_t sourceColorTexture, const glm::vec4& dstRectNDC) override;
  void readPixels(int x, int y, int width, int height, void* out) override;

 private:
  Ref<UniformBuffer> m_cameraUBO;

  unsigned int m_blitShader = 0;
  unsigned int m_quadVao = 0;
  unsigned int m_quadVbo = 0;

  unsigned int m_shadowShader = 0;
  Ref<UniformBuffer> m_lightUBO;
  unsigned int m_shadowMap = 0;

  unsigned int m_lightingShader = 0;
  Ref<UniformBuffer> m_pointLightUBO;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP
