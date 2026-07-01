#include "engine/renderer/RenderBackend.hpp"

#include "engine/renderer/opengl/OpenGLBackend.hpp"

namespace engine {

Scope<RenderBackend> RenderBackend::Create() {
  return CreateScope<OpenGLBackend>();
}

}  // namespace engine
