#include "engine/renderer/opengl/OpenGLRenderer.hpp"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace engine {

static unsigned int compileShader(unsigned int type, const char* source) {
  const unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  int success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    throw std::runtime_error(log);
  }

  return shader;
}

static unsigned int createShaderProgram() {
  constexpr const char* vertexSource = R"(
        #version 460 core

        layout(location = 0) in vec3 a_position;
        layout(location = 1) in vec4 a_color;

        uniform mat4 u_viewProjection;

        out vec4 v_color;

        void main() {
            v_color = a_color;
            gl_Position = u_viewProjection * vec4(a_position, 1.0);
        }
    )";

  constexpr const char* fragmentSource = R"(
        #version 460 core

        in vec4 v_color;
        out vec4 fragColor;

        void main() {
            fragColor = v_color;
        }
    )";

  const unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource);
  const unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

  const unsigned int program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  glDeleteShader(vs);
  glDeleteShader(fs);

  int success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  if (!success) {
    char log[1024];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    throw std::runtime_error(log);
  }

  return program;
}

OpenGLRenderer::OpenGLRenderer() { createResources(); }

OpenGLRenderer::~OpenGLRenderer() { destroyResources(); }

void OpenGLRenderer::createResources() {
  m_shader = createShaderProgram();

  glCreateVertexArrays(1, &m_vao);
  glCreateBuffers(1, &m_vbo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

  glBufferData(GL_ARRAY_BUFFER, 1024 * 1024, nullptr, GL_DYNAMIC_DRAW);

  constexpr int stride = sizeof(float) * 7;

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        reinterpret_cast<void*>(0));

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                        reinterpret_cast<void*>(sizeof(float) * 3));

  glBindVertexArray(0);
}

void OpenGLRenderer::destroyResources() {
  if (m_shader) glDeleteProgram(m_shader);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void OpenGLRenderer::beginFrame(int width, int height) {
  glViewport(0, 0, width, height);
  glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::endFrame() {}

void OpenGLRenderer::setCamera(const Camera& camera) {
  m_viewProjection = camera.viewProjection();
}

void OpenGLRenderer::saveScreenshot(const std::string& path, int width,
                                    int height) {
  std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

  // OpenGL reads bottom-to-top; PNG expects top-to-bottom.
  stbi_flip_vertically_on_write(1);

  if (stbi_write_png(path.c_str(), width, height, 3, pixels.data(),
                     width * 3)) {
    spdlog::info("Screenshot saved to {}", path);
  } else {
    spdlog::error("Failed to write screenshot to {}", path);
  }
}

void OpenGLRenderer::clear(const glm::vec4& color) {
  glClearColor(color.r, color.g, color.b, color.a);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderer::uploadAndDrawLines(const float* data, int vertexCount) {
  glUseProgram(m_shader);

  const int location = glGetUniformLocation(m_shader, "u_viewProjection");
  glUniformMatrix4fv(location, 1, GL_FALSE, &m_viewProjection[0][0]);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

  glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * 7 * sizeof(float), data);

  glDrawArrays(GL_LINES, 0, vertexCount);

  glBindVertexArray(0);
}

void OpenGLRenderer::drawLine(const glm::vec3& a, const glm::vec3& b,
                              const glm::vec4& color) {
  const float vertices[] = {a.x, a.y, a.z, color.r, color.g, color.b, color.a,
                            b.x, b.y, b.z, color.r, color.g, color.b, color.a};

  uploadAndDrawLines(vertices, 2);
}

void OpenGLRenderer::drawCube(const glm::vec3& center, const glm::vec3& size,
                              const glm::vec4& color) {
  const glm::vec3 h = size * 0.5f;

  const glm::vec3 p[8] = {
      center + glm::vec3{-h.x, -h.y, -h.z}, center + glm::vec3{h.x, -h.y, -h.z},
      center + glm::vec3{h.x, h.y, -h.z},   center + glm::vec3{-h.x, h.y, -h.z},

      center + glm::vec3{-h.x, -h.y, h.z},  center + glm::vec3{h.x, -h.y, h.z},
      center + glm::vec3{h.x, h.y, h.z},    center + glm::vec3{-h.x, h.y, h.z}};

  constexpr int edges[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                             6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

  std::vector<float> vertices;
  vertices.reserve(24 * 7);

  for (int index : edges) {
    const glm::vec3& v = p[index];

    vertices.push_back(v.x);
    vertices.push_back(v.y);
    vertices.push_back(v.z);
    vertices.push_back(color.r);
    vertices.push_back(color.g);
    vertices.push_back(color.b);
    vertices.push_back(color.a);
  }

  uploadAndDrawLines(vertices.data(), 24);
}

void OpenGLRenderer::drawGrid(float halfSize, float spacing,
                              const glm::vec4& color) {
  std::vector<float> vertices;

  for (float i = -halfSize; i <= halfSize; i += spacing) {
    const glm::vec3 a{i, 0.0f, -halfSize};
    const glm::vec3 b{i, 0.0f, halfSize};
    const glm::vec3 c{-halfSize, 0.0f, i};
    const glm::vec3 d{halfSize, 0.0f, i};

    for (const glm::vec3& v : {a, b, c, d}) {
      vertices.push_back(v.x);
      vertices.push_back(v.y);
      vertices.push_back(v.z);
      vertices.push_back(color.r);
      vertices.push_back(color.g);
      vertices.push_back(color.b);
      vertices.push_back(color.a);
    }
  }

  uploadAndDrawLines(vertices.data(), static_cast<int>(vertices.size() / 7));
}

}  // namespace engine