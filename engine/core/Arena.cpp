#include "engine/core/Arena.hpp"

#include <cstdint>
#include <cstdlib>
#include <new>
#include <stdexcept>

namespace engine {

Arena::Arena(std::size_t capacityBytes) : m_capacity(capacityBytes) {
  m_base = static_cast<std::byte*>(::operator new(capacityBytes));
}

Arena::~Arena() { ::operator delete(m_base); }

void* Arena::allocate(std::size_t size, std::size_t alignment) {
  const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(m_base);
  const std::uintptr_t current = base + m_offset;
  const std::uintptr_t aligned = (current + (alignment - 1)) &
                                 ~(static_cast<std::uintptr_t>(alignment) - 1);
  const std::size_t newOffset = (aligned - base) + size;

  if (newOffset > m_capacity) {
    throw std::runtime_error("Arena: out of memory");
  }

  m_offset = newOffset;
  return reinterpret_cast<void*>(aligned);
}

void Arena::reset() { m_offset = 0; }

}  // namespace engine
