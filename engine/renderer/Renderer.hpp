#ifndef ENGINE_RENDERER_RENDERER_HPP
#define ENGINE_RENDERER_RENDERER_HPP

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/core/JobSystem.hpp"
#include "engine/renderer/Camera.hpp"
#include "engine/renderer/CameraUniforms.hpp"
#include "engine/renderer/CommandBufferPool.hpp"
#include "engine/renderer/Framebuffer.hpp"
#include "engine/renderer/LightUniforms.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/RenderBackend.hpp"
#include "engine/renderer/RenderPass.hpp"
#include "engine/renderer/RenderQueue.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/renderer/TextureCube.hpp"
#include "engine/renderer/text/Font.hpp"
#include "engine/renderer/text/TextLayout.hpp"

namespace engine {

class Renderer {
 public:
  explicit Renderer(JobSystem& jobs);

  void beginFrame(int width, int height);
  void endFrame();

  void setCamera(const Camera& camera) {
    m_camera = makeCameraUniforms(camera.view(), camera.projection());
  }

  void setLightDirection(const glm::vec3& dir) { m_lightDir = dir; }

  void setPointLights(const std::vector<PointLight>& lights);

  RenderQueue& queue() { return m_lanes[0]->queue; }
  ResourceRegistry& registry() { return m_registry; }
  MeshHandle unitCubeMesh() const { return m_cubeMesh; }
  ShaderHandle meshShader() const { return m_meshShader; }

  // Generate `count` items in parallel; fn(queue, begin, end) runs on a worker
  // lane and records into that lane's thread-local queue.
  void generateMeshes(
      size_t count,
      const std::function<void(RenderQueue&, size_t begin, size_t end)>& fn);

  void saveScreenshot(const std::string& path, int width, int height);

  // Enqueue a screen-space text string (pixel coords, y-down) to be drawn over
  // the composited frame in endFrame.
  void drawText(const Font& font, std::string_view text, float x, float y,
                float scale, const glm::vec4& color);

  struct RenderStats {
    size_t drawCount = 0;
    int pointLights = 0;
    size_t laneCount = 0;
    glm::vec3 cameraPos{0.0f};
    glm::vec3 cameraFwd{0.0f, 0.0f, -1.0f};
  };
  RenderStats stats() const;

 private:
  static constexpr std::size_t kLaneArenaBytes = 8 * 1024 * 1024;
  static constexpr size_t kBatchSize = 128;

  void initBuiltins();

  struct TextBatch {
    uint32_t atlas;
    std::vector<TextQuad> quads;
    glm::vec4 color;
  };

  JobSystem& m_jobs;
  std::vector<Scope<WorkerBuffer>> m_lanes;
  std::vector<RenderEntry> m_merged;
  std::vector<TextBatch> m_textBatches;
  ResourceRegistry m_registry;
  Scope<RenderBackend> m_backend;
  CameraUniforms m_camera;
  Ref<Framebuffer> m_sceneFBO;
  RenderPass m_scenePass;
  RenderPass m_compositePass;
  Ref<Framebuffer> m_gbufferFBO;
  RenderPass m_gbufferPass;
  Ref<Framebuffer> m_shadowFBO;
  RenderPass m_shadowPass;
  glm::vec3 m_lightDir{glm::normalize(glm::vec3(0.4f, 0.8f, 0.3f))};
  LightUniforms m_light;
  static constexpr uint32_t kShadowSize = 2048;
  Ref<TextureCube> m_envMap;
  static constexpr uint32_t kEnvSize = 512;
  Ref<TextureCube> m_irradianceMap;
  Ref<TextureCube> m_prefilterMap;
  Ref<Framebuffer> m_brdfFBO;
  static constexpr uint32_t kIrradianceSize = 32;
  static constexpr uint32_t kPrefilterSize = 128;
  static constexpr int kPrefilterMips = 5;
  static constexpr uint32_t kBrdfSize = 512;
  Ref<Framebuffer> m_ssaoFBO;
  Ref<Framebuffer> m_ssaoBlurFBO;
  Ref<Framebuffer> m_bloomFBO[2];
  static constexpr int kBloomBlurPasses = 5;
  int m_pointLightCount = 0;
  int m_width = 0;
  int m_height = 0;
  MeshHandle m_cubeMesh = 0;
  ShaderHandle m_meshShader = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERER_HPP
