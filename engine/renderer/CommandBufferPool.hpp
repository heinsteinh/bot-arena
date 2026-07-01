#ifndef ENGINE_RENDERER_COMMANDBUFFERPOOL_HPP
#define ENGINE_RENDERER_COMMANDBUFFERPOOL_HPP

#include <cstddef>
#include <vector>

#include "engine/core/Arena.hpp"
#include "engine/core/Base.hpp"
#include "engine/renderer/RenderQueue.hpp"

namespace engine {

// One thread-local command buffer: a lane's arena and the queue that records
// commands into it. Heap-boxed by the Renderer because Arena is non-movable.
struct WorkerBuffer {
  explicit WorkerBuffer(std::size_t bytes) : arena(bytes), queue(arena) {}
  Arena arena;
  RenderQueue queue;
};

// Appends every lane's entries to `out`, in lane index order.
void mergeLaneEntries(const std::vector<Scope<WorkerBuffer>>& lanes,
                      std::vector<RenderEntry>& out);

}  // namespace engine

#endif  // ENGINE_RENDERER_COMMANDBUFFERPOOL_HPP
