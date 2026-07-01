#ifndef ENGINE_RENDERER_CAMERACONTROLLER_HPP
#define ENGINE_RENDERER_CAMERACONTROLLER_HPP

#include "engine/renderer/Camera.hpp"

namespace engine {

// Drives a camera from input state. Implementations own their camera and expose
// it through camera(); the host updates them once per frame and on resize.
class CameraController {
 public:
  virtual ~CameraController() = default;

  virtual void update(float dt) = 0;
  virtual void resize(float aspectRatio) = 0;

  virtual Camera& camera() = 0;
  virtual const Camera& camera() const = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_CAMERACONTROLLER_HPP
