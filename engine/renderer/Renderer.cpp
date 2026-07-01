#include "engine/renderer/Renderer.hpp"

#include <spdlog/spdlog.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <vector>

#include "engine/core/AssetPath.hpp"
#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

Renderer::Renderer(JobSystem& jobs)
    : m_jobs(jobs), m_backend(RenderBackend::Create()) {
  m_lanes.reserve(jobs.workerCount());
  for (size_t i = 0; i < jobs.workerCount(); ++i) {
    m_lanes.push_back(CreateScope<WorkerBuffer>(kLaneArenaBytes));
  }
  initBuiltins();

  m_sceneFBO = Framebuffer::Create(FramebufferSpec{});
  m_scenePass = RenderPass{m_sceneFBO, {0.08f, 0.09f, 0.11f, 1.0f}, true};
  m_compositePass = RenderPass{nullptr, {0.0f, 0.0f, 0.0f, 1.0f}, false};
}

void Renderer::initBuiltins() {
  // Unit cube: 6 faces x 4 verts, position(3) + normal(3).
  const float v[] = {// +X
                     1, -1, -1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
                     -1, 1, 1, 0, 0,
                     // -X
                     -1, -1, 1, -1, 0, 0, -1, 1, 1, -1, 0, 0, -1, 1, -1, -1, 0,
                     0, -1, -1, -1, -1, 0, 0,
                     // +Y
                     -1, 1, -1, 0, 1, 0, 1, 1, -1, 0, 1, 0, 1, 1, 1, 0, 1, 0,
                     -1, 1, 1, 0, 1, 0,
                     // -Y
                     -1, -1, 1, 0, -1, 0, 1, -1, 1, 0, -1, 0, 1, -1, -1, 0, -1,
                     0, -1, -1, -1, 0, -1, 0,
                     // +Z
                     -1, -1, 1, 0, 0, 1, 1, -1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1,
                     -1, 1, 1, 0, 0, 1,
                     // -Z
                     1, -1, -1, 0, 0, -1, -1, -1, -1, 0, 0, -1, -1, 1, -1, 0, 0,
                     -1, 1, 1, -1, 0, 0, -1};
  uint32_t idx[36];
  for (uint32_t f = 0; f < 6; ++f) {
    const uint32_t o = f * 4;
    const uint32_t b = f * 6;
    idx[b + 0] = o + 0;
    idx[b + 1] = o + 1;
    idx[b + 2] = o + 2;
    idx[b + 3] = o + 2;
    idx[b + 4] = o + 3;
    idx[b + 5] = o + 0;
  }

  auto va = VertexArray::Create();
  auto vb = VertexBuffer::Create(v, sizeof(v));
  vb->setLayout({
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float3, "a_normal"},
  });
  va->addVertexBuffer(vb);
  va->setIndexBuffer(IndexBuffer::Create(idx, 36));

  m_cubeMesh = m_registry.registerMesh(va);
  m_meshShader =
      m_registry.registerShader(Shader::Create(assetPath("shaders/mesh.glsl")));
}

void Renderer::beginFrame(int width, int height) {
  m_width = width;
  m_height = height;
  for (Scope<WorkerBuffer>& lane : m_lanes) {
    lane->arena.reset();
    lane->queue.clear();
  }
  m_sceneFBO->resize(static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height));
}

void Renderer::generateMeshes(
    size_t count, const std::function<void(RenderQueue&, size_t, size_t)>& fn) {
  m_jobs.parallelFor(count, kBatchSize,
                     [this, &fn](size_t begin, size_t end, size_t lane) {
                       fn(m_lanes[lane]->queue, begin, end);
                     });
}

void Renderer::endFrame() {
  m_merged.clear();
  mergeLaneEntries(m_lanes, m_merged);
  std::stable_sort(m_merged.begin(), m_merged.end(),
                   [](const RenderEntry& a, const RenderEntry& b) {
                     return a.sortKey < b.sortKey;
                   });

  // Scene pass -> HDR framebuffer.
  m_backend->beginPass(m_scenePass.target.get(), m_scenePass.clearColor,
                       m_scenePass.clearDepth, m_width, m_height);
  m_backend->execute(m_merged, m_camera, m_lanes[0]->arena, m_registry);

  // Composite pass -> default framebuffer.
  m_backend->beginPass(m_compositePass.target.get(), m_compositePass.clearColor,
                       m_compositePass.clearDepth, m_width, m_height);
  m_backend->blit(m_sceneFBO->colorAttachment(), {-1.0f, -1.0f, 1.0f, 1.0f});
}

void Renderer::saveScreenshot(const std::string& path, int width, int height) {
  std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
  m_backend->readPixels(0, 0, width, height, pixels.data());

  stbi_flip_vertically_on_write(1);
  if (stbi_write_png(path.c_str(), width, height, 3, pixels.data(),
                     width * 3)) {
    spdlog::info("Screenshot saved to {}", path);
  } else {
    spdlog::error("Failed to write screenshot to {}", path);
  }
}

}  // namespace engine
