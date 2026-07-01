#ifndef ENGINE_RENDERER_RENDERER_HPP
#define ENGINE_RENDERER_RENDERER_HPP

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/core/JobSystem.hpp"
#include "engine/renderer/Camera.hpp"
#include "engine/renderer/CameraUniforms.hpp"
#include "engine/renderer/CommandBufferPool.hpp"
#include "engine/renderer/Framebuffer.hpp"
#include "engine/renderer/LightUniforms.hpp"
#include "engine/renderer/MinimapRect.hpp"
#include "engine/renderer/RenderBackend.hpp"
#include "engine/renderer/RenderPass.hpp"
#include "engine/renderer/RenderQueue.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

class Renderer {
 public:
  explicit Renderer(JobSystem& jobs);

  void beginFrame(int width, int height);
  void endFrame();

  void setCamera(const Camera& camera) {
    m_camera = makeCameraUniforms(camera.view(), camera.projection());
  }

  void setMinimapCamera(const Camera& camera) {
    m_minimapCamera = makeCameraUniforms(camera.view(), camera.projection());
  }

  void setLightDirection(const glm::vec3& dir) { m_lightDir = dir; }

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

 private:
  static constexpr std::size_t kLaneArenaBytes = 8 * 1024 * 1024;
  static constexpr size_t kBatchSize = 128;

  void initBuiltins();

  JobSystem& m_jobs;
  std::vector<Scope<WorkerBuffer>> m_lanes;
  std::vector<RenderEntry> m_merged;
  ResourceRegistry m_registry;
  Scope<RenderBackend> m_backend;
  CameraUniforms m_camera;
  Ref<Framebuffer> m_sceneFBO;
  RenderPass m_scenePass;
  RenderPass m_compositePass;
  Ref<Framebuffer> m_minimapFBO;
  RenderPass m_minimapPass;
  CameraUniforms m_minimapCamera;
  static constexpr uint32_t kMinimapSize = 512;
  Ref<Framebuffer> m_shadowFBO;
  RenderPass m_shadowPass;
  glm::vec3 m_lightDir{glm::normalize(glm::vec3(0.4f, 0.8f, 0.3f))};
  LightUniforms m_light;
  static constexpr uint32_t kShadowSize = 2048;
  int m_width = 0;
  int m_height = 0;
  MeshHandle m_cubeMesh = 0;
  ShaderHandle m_meshShader = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERER_HPP
