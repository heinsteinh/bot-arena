#ifndef ENGINE_RENDERER_UNIFORMBUFFER_HPP
#define ENGINE_RENDERER_UNIFORMBUFFER_HPP

#include <cstdint>

#include "engine/core/Base.hpp"

namespace engine {

class UniformBuffer {
 public:
  virtual ~UniformBuffer() = default;
  virtual void setData(const void* data, uint32_t size,
                       uint32_t offset = 0) = 0;

  static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_UNIFORMBUFFER_HPP
