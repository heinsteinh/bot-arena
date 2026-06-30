#ifndef ENGINE_RENDERER_OPENGL_OPENGLRENDERER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLRENDERER_HPP

#include "engine/renderer/Camera.hpp"
#include "engine/renderer/Renderer.hpp"

namespace engine {

class OpenGLRenderer final : public Renderer {
 public:
  OpenGLRenderer();
  ~OpenGLRenderer() override;

  void beginFrame(int width, int height) override;
  void endFrame() override;
  void clear(const glm::vec4& color) override;

  void setCamera(const Camera& camera) override;
  void drawLine(const glm::vec3& a, const glm::vec3& b,
                const glm::vec4& color) override;
  void drawCube(const glm::vec3& center, const glm::vec3& size,
                const glm::vec4& color) override;
  void drawGrid(float halfSize, float spacing, const glm::vec4& color) override;

 private:
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_shader = 0;

  glm::mat4 m_viewProjection{1.0f};

  void createResources();
  void destroyResources();

  void uploadAndDrawLines(const float* data, int vertexCount);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLRENDERER_HPP
