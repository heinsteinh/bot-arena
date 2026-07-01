#ifndef ENGINE_RENDERER_LIGHTUNIFORMS_HPP
#define ENGINE_RENDERER_LIGHTUNIFORMS_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

// std140-compatible directional-light block uploaded once per frame at
// binding 1.
struct LightUniforms {
  glm::mat4 lightViewProj{1.0f};
  glm::vec4 lightDir{0.0f};
};

// Orthographic light-space view-projection. nearPlane/farPlane are positive
// distances; the eye is placed farPlane*0.5 behind the scene along -lightDir.
inline glm::mat4 makeLightViewProj(const glm::vec3& lightDir,
                                   const glm::vec3& center, float halfExtent,
                                   float nearPlane, float farPlane) {
  const glm::vec3 dir = glm::normalize(lightDir);
  const glm::vec3 eye = center - dir * (farPlane * 0.5f);
  const glm::mat4 view = glm::lookAt(eye, center, glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::mat4 proj = glm::ortho(-halfExtent, halfExtent, -halfExtent,
                                    halfExtent, nearPlane, farPlane);
  return proj * view;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_LIGHTUNIFORMS_HPP
