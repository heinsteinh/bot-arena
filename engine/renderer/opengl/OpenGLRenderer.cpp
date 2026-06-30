#include "engine/renderer/opengl/OpenGLRenderer.hpp"

#include <glad/glad.h>

namespace engine {

void OpenGLRenderer::beginFrame(int width, int height) {
  glViewport(0, 0, width, height);
  glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::endFrame() {}

void OpenGLRenderer::clear(const glm::vec4& color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

}  // namespace engine