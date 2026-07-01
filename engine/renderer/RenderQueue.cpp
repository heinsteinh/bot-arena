#include "engine/renderer/RenderQueue.hpp"

#include <algorithm>

namespace engine {

RenderQueue::RenderQueue(Arena& arena) : m_arena(arena) {}

void RenderQueue::submit(const RenderCommand& command) {
  RenderCommand* stored = m_arena.alloc<RenderCommand>();
  *stored = command;
  m_entries.push_back({stored->sortKey, stored});
}

void RenderQueue::clear() { m_entries.clear(); }

void RenderQueue::sort() {
  std::stable_sort(m_entries.begin(), m_entries.end(),
                   [](const RenderEntry& a, const RenderEntry& b) {
                     return a.sortKey < b.sortKey;
                   });
}

}  // namespace engine
