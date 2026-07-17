#ifndef ENGINE_SCENE_COMPONENTS_HPP
#define ENGINE_SCENE_COMPONENTS_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

namespace engine {

enum class ProjectionType { Perspective, Orthographic };

struct IDComponent {
  uint64_t id = 0;
};

struct TagComponent {
  std::string name;
};

struct TransformComponent {
  glm::vec3 translation{0.0f};
  glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};  // identity (w,x,y,z)
  glm::vec3 scale{1.0f};

  glm::mat4 localTransform() const {
    return glm::translate(glm::mat4(1.0f), translation) *
           glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
  }
};

struct CameraComponent {
  ProjectionType type = ProjectionType::Perspective;
  float fov = 45.0f;  // vertical FOV degrees (perspective)
  float perspNear = 0.1f;
  float perspFar = 1000.0f;
  float orthoSize = 10.0f;  // vertical extent (orthographic)
  float orthoNear = -1.0f;
  float orthoFar = 1.0f;
  float aspect = 16.0f / 9.0f;
  bool primary = true;
  bool fixedAspectRatio = false;
};

}  // namespace engine

#endif  // ENGINE_SCENE_COMPONENTS_HPP
