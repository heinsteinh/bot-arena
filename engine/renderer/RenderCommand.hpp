#ifndef ENGINE_RENDERER_RENDERCOMMAND_HPP
#define ENGINE_RENDERER_RENDERCOMMAND_HPP

#include <cstdint>
#include <glm/glm.hpp>

namespace engine {

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

inline uint64_t makeSortKey(RenderLayer layer, uint64_t sequence) {
  return (static_cast<uint64_t>(layer) << 56) |
         (sequence & 0x00FFFFFFFFFFFFFFull);
}

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERCOMMAND_HPP
