#ifndef ENGINE_RENDERER_RENDERCOMMAND_HPP
#define ENGINE_RENDERER_RENDERCOMMAND_HPP

#include <cstdint>
#include <glm/glm.hpp>

namespace engine {

using ShaderHandle = uint16_t;
using MeshHandle = uint16_t;
using MaterialHandle = uint16_t;

enum class RenderLayer : uint8_t { Grid = 0, Opaque = 1, Debug = 2, UI = 3 };
enum class RenderCommandType : uint8_t { Line, Cube };

struct RenderCommand {
  RenderCommandType type = RenderCommandType::Line;
  RenderLayer layer = RenderLayer::Debug;

  glm::vec3 position{0.0f};
  glm::vec3 scale{1.0f};
  glm::vec4 color{1.0f};

  glm::vec3 lineStart{0.0f};
  glm::vec3 lineEnd{0.0f};

  uint64_t sortKey = 0;
};

// [ layer:8 | shader:16 | material:16 | depth:24 ]
inline uint64_t makeSortKey(RenderLayer layer, uint16_t shader,
                            uint16_t material, uint32_t depth24) {
  return (static_cast<uint64_t>(layer) << 56) |
         (static_cast<uint64_t>(shader) << 40) |
         (static_cast<uint64_t>(material) << 24) |
         (static_cast<uint64_t>(depth24 & 0x00FFFFFFu));
}

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERCOMMAND_HPP
