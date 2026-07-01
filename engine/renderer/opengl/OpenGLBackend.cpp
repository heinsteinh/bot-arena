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

    const float PI = 3.14159265359;

    uniform sampler2D u_gAlbedo;
    uniform sampler2D u_gNormal;
    uniform sampler2D u_gWorldPos;
    uniform sampler2DShadow u_shadowMap;

    layout(std140, binding = 0) uniform Camera {
      mat4 u_view;
      mat4 u_projection;
      mat4 u_viewProjection;
      vec4 u_cameraPos;
    };

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

    float distributionGGX(vec3 N, vec3 H, float rough) {
      float a = rough * rough;
      float a2 = a * a;
      float ndh = max(dot(N, H), 0.0);
      float d = ndh * ndh * (a2 - 1.0) + 1.0;
      return a2 / (PI * d * d);
    }
    float geometrySchlickGGX(float ndv, float rough) {
      float r = rough + 1.0;
      float k = (r * r) / 8.0;
      return ndv / (ndv * (1.0 - k) + k);
    }
    float geometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
      return geometrySchlickGGX(max(dot(N, V), 0.0), rough) *
             geometrySchlickGGX(max(dot(N, L), 0.0), rough);
    }
    vec3 fresnelSchlick(float cosT, vec3 F0) {
      return F0 + (1.0 - F0) * pow(clamp(1.0 - cosT, 0.0, 1.0), 5.0);
    }

    vec3 brdf(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic,
              float rough, vec3 F0, vec3 radiance) {
      vec3 H = normalize(V + L);
      float ndl = max(dot(N, L), 0.0);
      float ndv = max(dot(N, V), 0.0);
      float D = distributionGGX(N, H, rough);
      float G = geometrySmith(N, V, L, rough);
      vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
      vec3 spec = (D * G * F) / (4.0 * ndv * ndl + 0.0001);
      vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
      return (kd * albedo / PI + spec) * radiance * ndl;
    }

    void main() {
      vec4 nSample = texture(u_gNormal, v_uv);
      vec3 N = nSample.xyz;
      if (dot(N, N) < 0.5) {          // no geometry: background
        fragColor = vec4(0.02, 0.02, 0.03, 1.0);
        return;
      }
      N = normalize(N);
      float rough = nSample.a;
      vec4 aSample = texture(u_gAlbedo, v_uv);
      vec3 albedo = aSample.rgb;
      float metallic = aSample.a;
      vec3 worldPos = texture(u_gWorldPos, v_uv).xyz;

      vec3 V = normalize(u_cameraPos.xyz - worldPos);
      vec3 F0 = mix(vec3(0.04), albedo, metallic);

      vec3 color = 0.03 * albedo;     // flat ambient

      // Directional light (shadowed).
      vec3 L = normalize(u_lightDir.xyz);
      float shadow = shadowPCF(worldPos, max(dot(N, L), 0.0));
      color += brdf(N, V, L, albedo, metallic, rough, F0, vec3(3.0)) *
               (1.0 - shadow);

      // Point lights.
      for (int i = 0; i < u_pointCount; ++i) {
        vec3 toL = u_points[i].positionRadius.xyz - worldPos;
        float dist = length(toL);
        float radius = u_points[i].positionRadius.w;
        vec3 Lp = toL / max(dist, 1e-4);
        float att = clamp(1.0 - dist / radius, 0.0, 1.0);
        att *= att;
        vec3 radiance = u_points[i].color.rgb * u_points[i].color.a * att;
        color += brdf(N, V, Lp, albedo, metallic, rough, F0, radiance);
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

}  // namespace

OpenGLBackend::OpenGLBackend() {
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
  if (m_blitShader) glDeleteProgram(m_blitShader);
  if (m_shadowShader) glDeleteProgram(m_shadowShader);
  if (m_lightingShader) glDeleteProgram(m_lightingShader);
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
      shader->setFloat("u_metallic", mat.metallic);
      shader->setFloat("u_roughness", mat.roughness);
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
