#include "engine/renderer/opengl/OpenGLShader.hpp"

#include <glad/glad.h>

#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <stdexcept>

namespace engine {

static uint32_t compile(GLenum type, const std::string& source) {
  const uint32_t shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);
  int ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    throw std::runtime_error(std::string("Shader compile error: ") + log);
  }
  return shader;
}

Ref<Shader> Shader::Create(const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file)
    throw std::runtime_error("Shader::Create: cannot open " + filepath);
  std::stringstream ss;
  ss << file.rdbuf();
  const ShaderSources sources = ParseSources(ss.str());

  std::string name = filepath;
  const size_t slash = name.find_last_of("/\\");
  if (slash != std::string::npos) name = name.substr(slash + 1);
  const size_t dot = name.rfind('.');
  if (dot != std::string::npos) name = name.substr(0, dot);

  return CreateRef<OpenGLShader>(name, sources.vertex, sources.fragment);
}

Ref<Shader> Shader::Create(const std::string& name, const std::string& vertex,
                           const std::string& fragment) {
  return CreateRef<OpenGLShader>(name, vertex, fragment);
}

OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertex,
                           const std::string& fragment)
    : m_name(name) {
  const uint32_t vs = compile(GL_VERTEX_SHADER, vertex);
  const uint32_t fs = compile(GL_FRAGMENT_SHADER, fragment);
  m_rendererID = glCreateProgram();
  glAttachShader(m_rendererID, vs);
  glAttachShader(m_rendererID, fs);
  glLinkProgram(m_rendererID);
  int ok = 0;
  glGetProgramiv(m_rendererID, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetProgramInfoLog(m_rendererID, sizeof(log), nullptr, log);
    throw std::runtime_error(std::string("Shader link error: ") + log);
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
}

OpenGLShader::~OpenGLShader() { glDeleteProgram(m_rendererID); }

void OpenGLShader::bind() const { glUseProgram(m_rendererID); }
void OpenGLShader::unbind() const { glUseProgram(0); }

int OpenGLShader::uniformLocation(const std::string& name) {
  auto it = m_uniformCache.find(name);
  if (it != m_uniformCache.end()) return it->second;
  const int location = glGetUniformLocation(m_rendererID, name.c_str());
  m_uniformCache[name] = location;
  return location;
}

void OpenGLShader::setInt(const std::string& name, int value) {
  glUniform1i(uniformLocation(name), value);
}
void OpenGLShader::setFloat(const std::string& name, float value) {
  glUniform1f(uniformLocation(name), value);
}
void OpenGLShader::setFloat3(const std::string& name, const glm::vec3& value) {
  glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}
void OpenGLShader::setFloat4(const std::string& name, const glm::vec4& value) {
  glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}
void OpenGLShader::setMat4(const std::string& name, const glm::mat4& value) {
  glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

}  // namespace engine
