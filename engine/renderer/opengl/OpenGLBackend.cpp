#include "engine/renderer/opengl/OpenGLBackend.hpp"

#include <glad/glad.h>

#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <vector>

#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/CameraUniforms.hpp"
#include "engine/renderer/Framebuffer.hpp"
#include "engine/renderer/LightUniforms.hpp"
#include "engine/renderer/PointLight.hpp"
#include "engine/renderer/ResourceRegistry.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/UniformBuffer.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

namespace {

constexpr int kFloatsPerVertex = 7;  // vec3 position + vec4 color

struct GpuPointLights {
  int32_t count = 0;
  int32_t pad[3] = {0, 0, 0};
  PointLight lights[32] = {};
};

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

unsigned int createBlitProgram() {
  constexpr const char* vs = R"(
    #version 460 core
    layout(location = 0) in vec2 a_position;   // unit quad [0,1]^2
    uniform vec4 u_rect;                        // {x0,y0,x1,y1} in NDC
    out vec2 v_uv;
    void main() {
      v_uv = a_position;
      vec2 ndc = mix(u_rect.xy, u_rect.zw, a_position);
      gl_Position = vec4(ndc, 0.0, 1.0);
    }
  )";
  constexpr const char* fs = R"(
    #version 460 core
    in vec2 v_uv;
    out vec4 fragColor;
    uniform sampler2D u_scene;
    void main() {
      vec3 c = texture(u_scene, v_uv).rgb;
      c = c / (c + vec3(1.0));        // Reinhard tonemap
      c = pow(c, vec3(1.0 / 2.2));    // gamma
      fragColor = vec4(c, 1.0);
    }
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

unsigned int createShadowProgram() {
  constexpr const char* vs = R"(
    #version 460 core
    layout(location = 0) in vec3 a_position;
    uniform mat4 u_lightViewProj;
    uniform mat4 u_transform;
    void main() {
      gl_Position = u_lightViewProj * u_transform * vec4(a_position, 1.0);
    }
  )";
  constexpr const char* fs = R"(
    #version 460 core
    void main() {}
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

unsigned int createLightingProgram() {
  constexpr const char* vs = R"(
    #version 460 core
    layout(location = 0) in vec2 a_position;   // unit quad [0,1]^2
    out vec2 v_uv;
    void main() {
      v_uv = a_position;
      gl_Position = vec4(a_position * 2.0 - 1.0, 0.0, 1.0);
    }
  )";
  constexpr const char* fs = R"(
    #version 460 core
    in vec2 v_uv;
    out vec4 fragColor;

    uniform sampler2D u_gAlbedo;
    uniform sampler2D u_gNormal;
    uniform sampler2D u_gWorldPos;
    uniform sampler2DShadow u_shadowMap;

    layout(std140, binding = 1) uniform Light {
      mat4 u_lightViewProj;
      vec4 u_lightDir;
    };

    struct PointLight { vec4 positionRadius; vec4 color; };
    layout(std140, binding = 2) uniform PointLights {
      int u_pointCount;
      PointLight u_points[32];
    };

    float shadowPCF(vec3 worldPos, float ndl) {
      vec4 lc = u_lightViewProj * vec4(worldPos, 1.0);
      vec3 p = lc.xyz / lc.w * 0.5 + 0.5;
      if (p.z > 1.0) return 0.0;
      float bias = max(0.0025 * (1.0 - ndl), 0.0008);
      float texel = 1.0 / 2048.0;
      float sum = 0.0;
      for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
          sum += texture(u_shadowMap,
                         vec3(p.xy + vec2(x, y) * texel, p.z - bias));
      return 1.0 - sum / 9.0;
    }

    void main() {
      vec3 N = texture(u_gNormal, v_uv).xyz;
      if (dot(N, N) < 0.5) {          // no geometry: background
        fragColor = vec4(0.02, 0.02, 0.03, 1.0);
        return;
      }
      N = normalize(N);
      vec3 albedo = texture(u_gAlbedo, v_uv).rgb;
      vec3 worldPos = texture(u_gWorldPos, v_uv).xyz;

      vec3 color = 0.15 * albedo;     // ambient

      vec3 L = normalize(u_lightDir.xyz);
      float ndl = max(dot(N, L), 0.0);
      float shadow = shadowPCF(worldPos, ndl);
      color += ndl * (1.0 - shadow) * albedo;

      for (int i = 0; i < u_pointCount; ++i) {
        vec3 toL = u_points[i].positionRadius.xyz - worldPos;
        float dist = length(toL);
        float radius = u_points[i].positionRadius.w;
        vec3 Lp = toL / max(dist, 1e-4);
        float att = clamp(1.0 - dist / radius, 0.0, 1.0);
        att *= att;
        float ndlp = max(dot(N, Lp), 0.0);
        color += att * ndlp * u_points[i].color.rgb * u_points[i].color.a *
                 albedo;
      }

      fragColor = vec4(color, 1.0);
    }
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

  m_blitShader = createBlitProgram();

  // Unit quad [0,1]^2 as a triangle strip: (0,0),(1,0),(0,1),(1,1).
  const float quad[] = {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
  glCreateVertexArrays(1, &m_quadVao);
  glCreateBuffers(1, &m_quadVbo);
  glBindVertexArray(m_quadVao);
  glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2,
                        reinterpret_cast<void*>(0));
  glBindVertexArray(0);

  m_shadowShader = createShadowProgram();
  m_lightUBO = UniformBuffer::Create(sizeof(LightUniforms), 1);

  m_lightingShader = createLightingProgram();
  m_pointLightUBO = UniformBuffer::Create(sizeof(GpuPointLights), 2);
}

OpenGLBackend::~OpenGLBackend() {
  if (m_shader) glDeleteProgram(m_shader);
  if (m_blitShader) glDeleteProgram(m_blitShader);
  if (m_shadowShader) glDeleteProgram(m_shadowShader);
  if (m_lightingShader) glDeleteProgram(m_lightingShader);
  if (m_vbo) glDeleteBuffers(1, &m_vbo);
  if (m_vao) glDeleteVertexArrays(1, &m_vao);
  if (m_quadVbo) glDeleteBuffers(1, &m_quadVbo);
  if (m_quadVao) glDeleteVertexArrays(1, &m_quadVao);
}

void OpenGLBackend::beginPass(Framebuffer* target, const glm::vec4& clearColor,
                              bool clearDepth, int viewportW, int viewportH) {
  if (target) {
    target->bind();  // binds the FBO and sets its own viewport
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, viewportW, viewportH);
  }
  glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
  if (clearDepth) {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  } else {
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

void OpenGLBackend::executeShadow(const std::vector<RenderEntry>& entries,
                                  const glm::mat4& lightViewProj,
                                  Arena& /*scratch*/,
                                  const ResourceRegistry& registry) {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glUseProgram(m_shadowShader);
  glUniformMatrix4fv(glGetUniformLocation(m_shadowShader, "u_lightViewProj"), 1,
                     GL_FALSE, glm::value_ptr(lightViewProj));
  const int transformLoc = glGetUniformLocation(m_shadowShader, "u_transform");
  for (const RenderEntry& entry : entries) {
    const RenderCommand& cmd = *entry.command;
    if (cmd.type != RenderCommandType::Mesh) continue;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE,
                       glm::value_ptr(cmd.transform));
    const Ref<VertexArray>& vao = registry.mesh(cmd.mesh);
    vao->bind();
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(vao->indexBuffer()->count()),
                   GL_UNSIGNED_INT, nullptr);
  }
  glDisable(GL_CULL_FACE);
}

void OpenGLBackend::setLight(const LightUniforms& light,
                             uint32_t shadowMapTexture) {
  m_lightUBO->setData(&light, sizeof(LightUniforms));
  m_shadowMap = shadowMapTexture;
}

void OpenGLBackend::execute(const std::vector<RenderEntry>& entries,
                            const CameraUniforms& camera, Arena& /*scratch*/,
                            const ResourceRegistry& registry) {
  m_cameraUBO->setData(&camera, sizeof(CameraUniforms));
  if (entries.empty()) return;

  // --- Mesh pass: sorted; skip redundant shader/material binds ---
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glBindTextureUnit(1, m_shadowMap);
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
      shader->setInt("u_shadowMap", 1);
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

void OpenGLBackend::executeGeometry(const std::vector<RenderEntry>& entries,
                                    const CameraUniforms& camera,
                                    Arena& /*scratch*/,
                                    const ResourceRegistry& registry) {
  m_cameraUBO->setData(&camera, sizeof(CameraUniforms));
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
      boundMaterial = 0xFFFF;
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
}

void OpenGLBackend::setPointLights(int count, const PointLight* lights) {
  GpuPointLights data;
  data.count = count < 32 ? count : 32;
  for (int i = 0; i < data.count; ++i) data.lights[i] = lights[i];
  m_pointLightUBO->setData(&data, sizeof(GpuPointLights));
}

void OpenGLBackend::lightingPass(uint32_t gAlbedo, uint32_t gNormal,
                                 uint32_t gWorldPos, uint32_t shadowMap) {
  glDisable(GL_DEPTH_TEST);
  glUseProgram(m_lightingShader);
  glBindTextureUnit(0, gAlbedo);
  glBindTextureUnit(1, gNormal);
  glBindTextureUnit(2, gWorldPos);
  glBindTextureUnit(3, shadowMap);
  glUniform1i(glGetUniformLocation(m_lightingShader, "u_gAlbedo"), 0);
  glUniform1i(glGetUniformLocation(m_lightingShader, "u_gNormal"), 1);
  glUniform1i(glGetUniformLocation(m_lightingShader, "u_gWorldPos"), 2);
  glUniform1i(glGetUniformLocation(m_lightingShader, "u_shadowMap"), 3);
  glBindVertexArray(m_quadVao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void OpenGLBackend::blit(uint32_t sourceColorTexture,
                         const glm::vec4& dstRectNDC) {
  glDisable(GL_DEPTH_TEST);
  glUseProgram(m_blitShader);
  glBindTextureUnit(0, sourceColorTexture);
  glUniform1i(glGetUniformLocation(m_blitShader, "u_scene"), 0);
  glUniform4f(glGetUniformLocation(m_blitShader, "u_rect"), dstRectNDC.x,
              dstRectNDC.y, dstRectNDC.z, dstRectNDC.w);
  glBindVertexArray(m_quadVao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void OpenGLBackend::readPixels(int x, int y, int width, int height, void* out) {
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, out);
}

}  // namespace engine
