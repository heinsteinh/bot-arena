#ifndef ENGINE_RENDERER_CAMERAUNIFORMS_HPP
#define ENGINE_RENDERER_CAMERAUNIFORMS_HPP

#include <glm/glm.hpp>

namespace engine {

// std140-compatible camera block uploaded once per frame at binding 0.
struct CameraUniforms {
  glm::mat4 view{1.0f};
  glm::mat4 projection{1.0f};
  glm::mat4 viewProjection{1.0f};
  glm::vec4 cameraPosition{0.0f};
  glm::mat4 invViewProjection{1.0f};
};

inline CameraUniforms makeCameraUniforms(const glm::mat4& view,
                                         const glm::mat4& projection) {
  CameraUniforms u;
  u.view = view;
  u.projection = projection;
  u.viewProjection = projection * view;
  u.cameraPosition = glm::vec4(glm::vec3(glm::inverse(view)[3]), 1.0f);
  u.invViewProjection = glm::inverse(u.viewProjection);
  return u;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_CAMERAUNIFORMS_HPP
