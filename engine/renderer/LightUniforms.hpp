#ifndef ENGINE_RENDERER_LIGHTUNIFORMS_HPP
#define ENGINE_RENDERER_LIGHTUNIFORMS_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

// std140-compatible directional-light block uploaded once per frame at
// binding 1.
struct LightUniforms {
  glm::mat4 cascadeViewProj[3];
  glm::vec4 cascadeSplits{0.0f};  // x,y,z = far view-depth of cascades 0,1,2
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

// One cascade's light-space view-projection: a stable square ortho that bounds
// the camera sub-frustum between view-space depths [nearD, farD]. Uses only the
// camera proj/view (no explicit near/far) by mapping depth -> NDC z directly.
inline glm::mat4 makeCascadeViewProj(const glm::vec3& lightDir,
                                     const glm::mat4& camView,
                                     const glm::mat4& camProj, float nearD,
                                     float farD) {
  auto ndcZ = [&](float d) {
    return (camProj[2][2] * (-d) + camProj[3][2]) / d;  // clip.z / clip.w
  };
  const glm::mat4 invVP = glm::inverse(camProj * camView);
  const float zN = ndcZ(nearD);
  const float zF = ndcZ(farD);
  glm::vec3 corners[8];
  int n = 0;
  for (int x = 0; x < 2; ++x)
    for (int y = 0; y < 2; ++y)
      for (int s = 0; s < 2; ++s) {
        const float z = (s == 0) ? zN : zF;
        glm::vec4 p =
            invVP * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f);
        corners[n++] = glm::vec3(p) / p.w;
      }
  glm::vec3 center(0.0f);
  for (int i = 0; i < 8; ++i) center += corners[i];
  center /= 8.0f;
  float radius = 0.0f;
  for (int i = 0; i < 8; ++i)
    radius = glm::max(radius, glm::length(corners[i] - center));
  // lightDir points TOWARD the light, so the shadow eye sits on the light side
  // (center + dir), looking down at the slice. up guards against an overhead
  // sun.
  const glm::vec3 dir = glm::normalize(lightDir);
  const glm::vec3 up = glm::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                               : glm::vec3(0, 1, 0);
  const glm::mat4 view =
      glm::lookAt(center + dir * (radius * 2.0f), center, up);
  const glm::mat4 proj =
      glm::ortho(-radius, radius, -radius, radius, 0.1f, radius * 4.0f);
  return proj * view;
}

}  // namespace engine

#endif  // ENGINE_RENDERER_LIGHTUNIFORMS_HPP
