#include "engine/renderer/opengl/OpenGLBackend.hpp"

#include <glad/glad.h>

#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/CameraUniforms.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/UniformBuffer.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

namespace {

constexpr int kFloatsPerVertex = 7;  // vec3 position + vec4 color

unsigned int compileShader(unsigned int type, const char* source) {
  const unsigned int shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);
  int ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    throw std::runtime_error(log);
  }
  return shader;
}

unsigned int createProgram() {
  constexpr const char* vs = R"(
    #version 460 core
    layout(location = 0) in vec3 a_position;
    layout(location = 1) in vec4 a_color;
    layout(std140, binding = 0) uniform Camera {
      mat4 u_view;
      mat4 u_projection;
      mat4 u_viewProjection;
      vec4 u_cameraPos;
    };
    out vec4 v_color;
    void main() {
      v_color = a_color;
      gl_Position = u_viewProjection * vec4(a_position, 1.0);
    }
  )";
  constexpr const char* fs = R"(
    #version 460 core
    in vec4 v_color;
    out vec4 fragColor;
    void main() { fragColor = v_color; }
  )";
  const unsigned int v = compileShader(GL_VERTEX_SHADER, vs);
  const unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);
  const unsigned int program = glCreateProgram();
  glAttachShader(program, v);
  glAttachShader(program, f);
  glLinkProgram(program);
  glDeleteShader(v);
  glDeleteShader(f);
  int ok = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    throw std::runtime_error(log);
  }
  return program;
}

// Appends one vertex (pos + color) to out.
void pushVertex(std::vector<float>& out, const glm::vec3& p,
                const glm::vec4& c) {
  out.push_back(p.x);
  out.push_back(p.y);
  out.push_back(p.z);
  out.push_back(c.r);
  out.push_back(c.g);
  out.push_back(c.b);
  out.push_back(c.a);
}

void expandCube(std::vector<float>& out, const RenderCommand& cmd) {
  const glm::vec3 h = cmd.scale * 0.5f;
  const glm::vec3 c = cmd.position;
  const glm::vec3 p[8] = {
      c + glm::vec3{-h.x, -h.y, -h.z}, c + glm::vec3{h.x, -h.y, -h.z},
      c + glm::vec3{h.x, h.y, -h.z},   c + glm::vec3{-h.x, h.y, -h.z},
      c + glm::vec3{-h.x, -h.y, h.z},  c + glm::vec3{h.x, -h.y, h.z},
      c + glm::vec3{h.x, h.y, h.z},    c + glm::vec3{-h.x, h.y, h.z}};
  const int edges[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                         6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
  for (int idx : edges) pushVertex(out, p[idx], cmd.color);
}

}  // namespace

OpenGLBackend::OpenGLBackend() {
  m_shader = createProgram();
  glCreateVertexArrays(1, &m_vao);
  glCreateBuffers(1, &m_vbo);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  m_vboCapacityBytes = 1024 * 1024;
  glBufferData(GL_ARRAY_BUFFER, m_vboCapacityBytes, nullptr, GL_DYNAMIC_DRAW);

  const int stride = sizeof(float) * kFloatsPerVertex;
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride,
                        reinterpret_cast<void*>(0));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride,
                        reinterpret_cast<void*>(sizeof(float) * 3));
  glBindVertexArray(0);

  m_cameraUBO = UniformBuffer::Create(sizeof(CameraUniforms), 0);
}

OpenGLBackend::~OpenGLBackend() {
  if (m_shader) glDeleteProgram(m_shader);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

void OpenGLBackend::beginFrame(int width, int height) {
  glViewport(0, 0, width, height);
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLBackend::execute(const std::vector<RenderEntry>& entries,
                            const CameraUniforms& camera, Arena& /*scratch*/,
                            const ResourceRegistry& registry) {
  m_cameraUBO->setData(&camera, sizeof(CameraUniforms));
  if (entries.empty()) return;

  // --- Mesh pass: sorted; skip redundant shader/material binds ---
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  ShaderHandle boundShader = 0xFFFF;
  MaterialHandle boundMaterial = 0xFFFF;
  Shader* shader = nullptr;
  for (const RenderEntry& entry : entries) {
    const RenderCommand& cmd = *entry.command;
    if (cmd.type != RenderCommandType::Mesh) continue;

    const Material& mat = registry.material(cmd.material);
    if (mat.shader != boundShader) {
      shader = registry.shader(mat.shader).get();
      shader->bind();
      boundShader = mat.shader;
      boundMaterial = 0xFFFF;  // force material re-set under the new shader
    }
    if (cmd.material != boundMaterial) {
      shader->setFloat4("u_baseColor", mat.baseColor);
      boundMaterial = cmd.material;
    }
    shader->setMat4("u_transform", cmd.transform);

    const Ref<VertexArray>& vao = registry.mesh(cmd.mesh);
    vao->bind();
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(vao->indexBuffer()->count()),
                   GL_UNSIGNED_INT, nullptr);
  }
  glDisable(GL_CULL_FACE);

  // --- Line pass: batch Line + wireframe Cube into one draw ---
  std::vector<float> vertices;
  for (const RenderEntry& entry : entries) {
    const RenderCommand& cmd = *entry.command;
    if (cmd.type == RenderCommandType::Line) {
      pushVertex(vertices, cmd.lineStart, cmd.color);
      pushVertex(vertices, cmd.lineEnd, cmd.color);
    } else if (cmd.type == RenderCommandType::Cube) {
      expandCube(vertices, cmd);
    }
  }
  if (vertices.empty()) return;

  glUseProgram(m_shader);

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

  const int bytes = static_cast<int>(vertices.size() * sizeof(float));
  if (bytes > m_vboCapacityBytes) {
    glBufferData(GL_ARRAY_BUFFER, bytes, nullptr, GL_DYNAMIC_DRAW);
    m_vboCapacityBytes = bytes;
  }
  glBufferSubData(GL_ARRAY_BUFFER, 0, bytes, vertices.data());

  const int vertexCount = static_cast<int>(vertices.size() / kFloatsPerVertex);
  glDrawArrays(GL_LINES, 0, vertexCount);
  glBindVertexArray(0);
}

void OpenGLBackend::endFrame() {}

void OpenGLBackend::readPixels(int x, int y, int width, int height, void* out) {
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, out);
}

}  // namespace engine
