#ifndef ENGINE_RENDERER_RENDERER_HPP
#define ENGINE_RENDERER_RENDERER_HPP

#include <glm/glm.hpp>
#include <string>

#include "engine/core/Arena.hpp"
#include "engine/core/Base.hpp"
#include "engine/renderer/RenderBackend.hpp"
#include "engine/renderer/RenderQueue.hpp"
#include "engine/renderer/ResourceRegistry.hpp"

namespace engine {

class Renderer {
 public:
  Renderer();

  void beginFrame(int width, int height);
  void endFrame();

  void setViewProjection(const glm::mat4& viewProjection) {
    m_viewProjection = viewProjection;
  }

  RenderQueue& queue() { return m_queue; }
  ResourceRegistry& registry() { return m_registry; }
  MeshHandle unitCubeMesh() const { return m_cubeMesh; }
  ShaderHandle meshShader() const { return m_meshShader; }

  void saveScreenshot(const std::string& path, int width, int height);

 private:
  static constexpr std::size_t kArenaBytes = 8 * 1024 * 1024;

  void initBuiltins();

  Arena m_arena;
  RenderQueue m_queue;
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
