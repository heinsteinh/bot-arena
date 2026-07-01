#ifndef ENGINE_RENDERER_RENDERER_HPP
#define ENGINE_RENDERER_RENDERER_HPP

#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/core/JobSystem.hpp"
#include "engine/renderer/CommandBufferPool.hpp"
#include "engine/renderer/RenderBackend.hpp"
#include "engine/renderer/RenderQueue.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

class Renderer {
 public:
  explicit Renderer(JobSystem& jobs);

  void beginFrame(int width, int height);
  void endFrame();

  void setViewProjection(const glm::mat4& viewProjection) {
    m_viewProjection = viewProjection;
  }

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
  glm::mat4 m_viewProjection{1.0f};
  int m_width = 0;
  int m_height = 0;
  MeshHandle m_cubeMesh = 0;
  ShaderHandle m_meshShader = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERER_HPP
