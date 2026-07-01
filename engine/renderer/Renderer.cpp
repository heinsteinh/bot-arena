#include "engine/renderer/Renderer.hpp"

#include <spdlog/spdlog.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <vector>

#include "engine/core/AssetPath.hpp"
#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

Renderer::Renderer()
    : m_arena(kArenaBytes),
      m_queue(m_arena),
      m_backend(RenderBackend::Create()) {
  initBuiltins();
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
  m_arena.reset();
  m_queue.clear();
  m_backend->beginFrame(width, height);
}

void Renderer::endFrame() {
  m_queue.sort();
  m_backend->execute(m_queue.entries(), m_viewProjection, m_arena, m_registry);
  m_backend->endFrame();
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
