#include "engine/renderer/CommandBufferPool.hpp"

namespace engine {

void mergeLaneEntries(const std::vector<Scope<WorkerBuffer>>& lanes,
                      std::vector<RenderEntry>& out) {
  for (const Scope<WorkerBuffer>& lane : lanes) {
    const std::vector<RenderEntry>& entries = lane->queue.entries();
    out.insert(out.end(), entries.begin(), entries.end());
  }
}

}  // namespace engine
