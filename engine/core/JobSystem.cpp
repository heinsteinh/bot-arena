#include "engine/core/JobSystem.hpp"

#include <algorithm>

namespace engine {

JobSystem::JobSystem(std::optional<unsigned> workerThreads) {
  unsigned n;
  if (workerThreads.has_value()) {
    n = *workerThreads;
  } else {
    const unsigned hw = std::thread::hardware_concurrency();
    n = hw > 1 ? hw - 1 : 0;
  }
  m_workerCount = static_cast<size_t>(n) + 1;  // + main lane

  for (unsigned i = 0; i < n; ++i) {
    m_threads.emplace_back([this, i] { workerLoop(i + 1); });
  }
}

JobSystem::~JobSystem() {
  {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_stop = true;
  }
  m_work.notify_all();
  for (std::thread& t : m_threads) t.join();
}

void JobSystem::workerLoop(size_t lane) {
  for (;;) {
    Batch batch;
    const std::function<void(size_t, size_t, size_t)>* fn;
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      m_work.wait(lk, [this] { return m_stop || !m_batches.empty(); });
      if (m_batches.empty()) {
        if (m_stop) return;
        continue;
      }
      batch = m_batches.back();
      m_batches.pop_back();
      fn = m_fn;
    }
    (*fn)(batch.begin, batch.end, lane);
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      if (--m_remaining == 0) m_done.notify_all();
    }
  }
}

void JobSystem::parallelFor(
    size_t count, size_t batchSize,
    const std::function<void(size_t, size_t, size_t)>& fn) {
  if (count == 0) return;
  if (batchSize == 0) batchSize = 1;

  std::vector<Batch> batches;
  for (size_t s = 0; s < count; s += batchSize) {
    batches.push_back({s, std::min(s + batchSize, count)});
  }

  {
    std::unique_lock<std::mutex> lk(m_mutex);
    m_fn = &fn;
    m_batches = std::move(batches);
    m_remaining = m_batches.size();
  }
  m_work.notify_all();

  // Main thread (lane 0) also drains batches.
  for (;;) {
    Batch batch;
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      if (m_batches.empty()) break;
      batch = m_batches.back();
      m_batches.pop_back();
    }
    fn(batch.begin, batch.end, 0);
    {
      std::unique_lock<std::mutex> lk(m_mutex);
      if (--m_remaining == 0) m_done.notify_all();
    }
  }

  std::unique_lock<std::mutex> lk(m_mutex);
  m_done.wait(lk, [this] { return m_remaining == 0; });
  m_fn = nullptr;
}

}  // namespace engine
