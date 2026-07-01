#ifndef ENGINE_RENDERER_RENDERQUEUE_HPP
#define ENGINE_RENDERER_RENDERQUEUE_HPP

#include <cstdint>
#include <vector>

#include "engine/core/Arena.hpp"
#include "engine/renderer/RenderCommand.hpp"

namespace engine {

struct RenderEntry {
  uint64_t sortKey;
  const RenderCommand* command;
};

class RenderQueue {
 public:
  explicit RenderQueue(Arena& arena);

  void submit(const RenderCommand& command);
  void clear();
  void sort();
  const std::vector<RenderEntry>& entries() const { return m_entries; }

 private:
  Arena& m_arena;
  std::vector<RenderEntry> m_entries;
  uint64_t m_sequence = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERQUEUE_HPP
