#ifndef ENGINE_RENDERER_SHADER_HPP
#define ENGINE_RENDERER_SHADER_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <sstream>
#include <string>

#include "engine/core/Base.hpp"

namespace engine {

struct ShaderSources {
  std::string vertex;
  std::string fragment;
};

class Shader {
 public:
  virtual ~Shader() = default;

  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual void setInt(const std::string& name, int value) = 0;
  virtual void setFloat(const std::string& name, float value) = 0;
  virtual void setFloat3(const std::string& name, const glm::vec3& value) = 0;
  virtual void setFloat4(const std::string& name, const glm::vec4& value) = 0;
  virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
  virtual const std::string& name() const = 0;

  static Ref<Shader> Create(const std::string& filepath);
  static Ref<Shader> Create(const std::string& name, const std::string& vertex,
                            const std::string& fragment);

  static ShaderSources ParseSources(const std::string& source) {
    ShaderSources out;
    enum class Stage { None, Vertex, Fragment } stage = Stage::None;
    std::istringstream stream(source);
    std::ostringstream vertex, fragment;
    std::string line;
    while (std::getline(stream, line)) {
      if (line.rfind("#type", 0) == 0) {
        if (line.find("vertex") != std::string::npos) {
          stage = Stage::Vertex;
        } else if (line.find("fragment") != std::string::npos) {
          stage = Stage::Fragment;
        } else {
          stage = Stage::None;
        }
        continue;
      }
      if (stage == Stage::Vertex) {
        vertex << line << '\n';
      } else if (stage == Stage::Fragment) {
        fragment << line << '\n';
      }
    }
    out.vertex = vertex.str();
    out.fragment = fragment.str();
    return out;
  }
};

}  // namespace engine

#endif  // ENGINE_RENDERER_SHADER_HPP
