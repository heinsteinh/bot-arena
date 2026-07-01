#ifndef ENGINE_CORE_JOBSYSTEM_HPP
#define ENGINE_CORE_JOBSYSTEM_HPP

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace engine {

// Fixed thread pool. parallelFor splits work into batches run across lanes;
// lane 0 is the calling (main) thread, lanes 1..N are worker threads.
class JobSystem {
 public:
  explicit JobSystem(std::optional<unsigned> workerThreads = std::nullopt);
  ~JobSystem();

  JobSystem(const JobSystem&) = delete;
  JobSystem& operator=(const JobSystem&) = delete;

  size_t workerCount() const { return m_workerCount; }

  void parallelFor(
      size_t count, size_t batchSize,
      const std::function<void(size_t begin, size_t end, size_t lane)>& fn);

 private:
  struct Batch {
    size_t begin;
    size_t end;
  };

  void workerLoop(size_t lane);

  size_t m_workerCount = 1;
  std::vector<std::thread> m_threads;

  std::mutex m_mutex;
  std::condition_variable m_work;
  std::condition_variable m_done;
  std::vector<Batch> m_batches;  // popped from the back
  const std::function<void(size_t, size_t, size_t)>* m_fn = nullptr;
  size_t m_remaining = 0;
  bool m_stop = false;
};

}  // namespace engine

#endif  // ENGINE_CORE_JOBSYSTEM_HPP
