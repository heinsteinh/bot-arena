#ifndef ENGINE_RENDERER_FRAMEBUFFER_HPP
#define ENGINE_RENDERER_FRAMEBUFFER_HPP

#include <cstdint>

#include "engine/core/Base.hpp"

namespace engine {

struct FramebufferSpec {
  uint32_t width = 1280;
  uint32_t height = 720;
  bool hdr = true;
  bool depthOnly = false;
};

class Framebuffer {
 public:
  virtual ~Framebuffer() = default;
  virtual void bind() = 0;
  virtual void unbind() = 0;
  virtual void resize(uint32_t width, uint32_t height) = 0;
  virtual uint32_t colorAttachment() const = 0;
  virtual uint32_t depthAttachment() const = 0;

  static Ref<Framebuffer> Create(const FramebufferSpec& spec);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_FRAMEBUFFER_HPP
