#include "engine/renderer/Renderer.hpp"

#include <spdlog/spdlog.h>
#include <stb_image_write.h>

#include <vector>

namespace engine {

Renderer::Renderer()
    : m_arena(kArenaBytes),
      m_queue(m_arena),
      m_backend(RenderBackend::Create()) {}

void Renderer::beginFrame(int width, int height) {
  m_width = width;
  m_height = height;
  m_arena.reset();
  m_queue.clear();
  m_backend->beginFrame(width, height);
}

void Renderer::endFrame() {
  m_queue.sort();
  m_backend->execute(m_queue.entries(), m_viewProjection, m_arena);
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
