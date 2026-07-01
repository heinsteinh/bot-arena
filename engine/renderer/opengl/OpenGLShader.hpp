#ifndef ENGINE_RENDERER_OPENGL_OPENGLSHADER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLSHADER_HPP

#include <string>
#include <unordered_map>

#include "engine/renderer/Shader.hpp"

namespace engine {

class OpenGLShader final : public Shader {
 public:
  OpenGLShader(const std::string& name, const std::string& vertex,
               const std::string& fragment);
  ~OpenGLShader() override;

  void bind() const override;
  void unbind() const override;
  void setInt(const std::string& name, int value) override;
  void setFloat(const std::string& name, float value) override;
  void setFloat3(const std::string& name, const glm::vec3& value) override;
  void setFloat4(const std::string& name, const glm::vec4& value) override;
  void setMat4(const std::string& name, const glm::mat4& value) override;
  const std::string& name() const override { return m_name; }

 private:
  int uniformLocation(const std::string& name);
  uint32_t m_rendererID = 0;
  std::string m_name;
  std::unordered_map<std::string, int> m_uniformCache;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLSHADER_HPP
