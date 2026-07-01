#ifndef ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP

#include "engine/core/Base.hpp"
#include "engine/renderer/Framebuffer.hpp"
#include "engine/renderer/RenderBackend.hpp"
#include "engine/renderer/UniformBuffer.hpp"

namespace engine {

class OpenGLBackend final : public RenderBackend {
 public:
  OpenGLBackend();
  ~OpenGLBackend() override;

  void beginFrame(int width, int height) override;
  void execute(const std::vector<RenderEntry>& entries,
               const CameraUniforms& camera, Arena& scratch,
               const ResourceRegistry& registry) override;
  void endFrame() override;
  void readPixels(int x, int y, int width, int height, void* out) override;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_shader = 0;
  int m_vboCapacityBytes = 0;
  Ref<UniformBuffer> m_cameraUBO;

  Ref<Framebuffer> m_sceneFBO;
  unsigned int m_compositeShader = 0;
  unsigned int m_fsVao = 0;
  unsigned int m_fsVbo = 0;
  int m_width = 0;
  int m_height = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLBACKEND_HPP
