#ifndef ENGINE_CORE_ARENA_HPP
#define ENGINE_CORE_ARENA_HPP

#include <cstddef>

namespace engine {

// Linear/bump allocator over one heap block. Allocations move a single offset
// forward; reset() rewinds it (no destructors run — POD use only).
class Arena {
 public:
  explicit Arena(std::size_t capacityBytes);
  ~Arena();

  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  void* allocate(std::size_t size,
                 std::size_t alignment = alignof(std::max_align_t));

  template <typename T>
  T* alloc(std::size_t count = 1) {
    return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
  }

  void reset();
  std::size_t used() const { return m_offset; }
  std::size_t capacity() const { return m_capacity; }

 private:
  std::byte* m_base = nullptr;
  std::size_t m_capacity = 0;
  std::size_t m_offset = 0;
};

}  // namespace engine

#endif  // ENGINE_CORE_ARENA_HPP
