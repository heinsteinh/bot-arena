#ifndef ENGINE_SCENE_MESHCOMPONENT_HPP
#define ENGINE_SCENE_MESHCOMPONENT_HPP

#include "engine/renderer/RenderCommand.hpp"

namespace engine {

struct MeshComponent {
  MeshHandle mesh = 0;
  MaterialHandle material = 0;
};

}  // namespace engine

#endif  // ENGINE_SCENE_MESHCOMPONENT_HPP
