# RHI Foundation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the immediate-mode debug renderer with a backend-abstracted RHI layer (RenderingAPI, Buffer/VertexArray/Shader/Texture/UniformBuffer, deferred-ready FrameBuffer + RenderPass, high-level Renderer + DebugRenderer) and an OpenGL 4.6 backend, proven by an offscreen-HDR textured-cube demo.

**Architecture:** Each RHI resource is an abstract base with a static `Create()` factory that switches on `RenderingAPI::api()` and returns a `Ref<T>` (`std::shared_ptr`). `RenderingAPI`, `VertexBuffer`, `IndexBuffer`, `VertexArray`, `Shader`, `Texture2D`, `UniformBuffer`, `FrameBuffer` get `OpenGL*` implementations. `Renderer`, `RenderPass`, `DebugRenderer` are concrete and backend-agnostic. New files are added and compiled alongside the existing renderer; the app is rewired to the new stack only in the final integration tasks, then the old renderer is deleted.

**Tech Stack:** C++20, OpenGL 4.6 core (DSA) via GLAD, SDL3, glm, stb (image + image_write), spdlog, Catch2 (new, for unit tests), CMake + vcpkg.

## Global Constraints

- C++20, 2-space indent, Google-style clang-format (`.clang-format` present; a pre-commit hook runs clang-format — run it before every commit).
- No AI attribution in commits/code/docs. Conventional Commits, subject ≤72 chars, imperative mood, capitalized, no trailing period.
- Include-guard style: `#ifndef ENGINE_..._HPP` for engine headers (match existing files); `#pragma once` is used in a few core headers (`Layer.hpp`, `Time.hpp`) — prefer the `#ifndef` guards for new files.
- Namespace: all engine code in `namespace engine`; game code in `namespace game`.
- `Ref<T> = std::shared_ptr<T>`, `Scope<T> = std::unique_ptr<T>` (defined in Task 1).
- OpenGL is 4.6 core; DSA calls (`glCreate*`, `glVertexArray*`, `glTextureStorage2D`, `glNamedBufferData`, …) are available and preferred.
- Build: `cmake --build build`. Run tests: `ctest --test-dir build --output-on-failure`.
- Screenshot verification: `cd build && BOTARENA_SCREENSHOT=<path> ./bot_arena` renders one frame to a PNG and exits.

---

## File Structure

**Created (engine):**
- `engine/core/Base.hpp` — `Ref`/`Scope` aliases + `CreateRef`/`CreateScope`.
- `engine/core/AssetPath.hpp` — `assetPath(rel)` using the `BOTARENA_ASSET_DIR` define.
- `engine/renderer/RenderingAPI.hpp` — API enum + abstract low-level executor.
- `engine/renderer/Buffer.hpp` — `ShaderDataType`, `BufferLayout`, abstract `VertexBuffer`/`IndexBuffer`.
- `engine/renderer/VertexArray.hpp` — abstract `VertexArray`.
- `engine/renderer/Shader.hpp` — abstract `Shader` + header-only `#type` parser.
- `engine/renderer/Texture.hpp` — formats + abstract `Texture`/`Texture2D`.
- `engine/renderer/UniformBuffer.hpp` — abstract `UniformBuffer`.
- `engine/renderer/FrameBuffer.hpp` — specs (MRT/HDR) + abstract `FrameBuffer`.
- `engine/renderer/RenderPass.hpp` + `.cpp` — concrete pass (spec holder).
- `engine/renderer/Renderer.hpp` + `.cpp` — high-level renderer (replaces old).
- `engine/renderer/DebugRenderer.hpp` + `.cpp` — batched line drawing on the RHI.
- `engine/renderer/opengl/OpenGL{RenderingAPI,Buffer,VertexArray,Shader,Texture,UniformBuffer,Framebuffer}.{hpp,cpp}`.

**Created (assets/tests):**
- `assets/shaders/{textured,debug_line,composite}.glsl`
- `tests/test_buffer_layout.cpp`, `tests/test_shader_parser.cpp`

**Modified:**
- `CMakeLists.txt` — new sources, `BOTARENA_ASSET_DIR` define, Catch2 test target.
- `vcpkg.json` — add `catch2`.
- `engine/core/Layer.hpp` — include path unchanged (`engine/renderer/Renderer.hpp`), signature unchanged.
- `engine/core/Application.{hpp,cpp}` — own new `Renderer`, simplified loop.
- `game/BotArenaGame.{hpp,cpp}` — demo using Renderer + DebugRenderer.

**Deleted (Task 16):**
- `engine/renderer/opengl/OpenGLRenderer.{hpp,cpp}` and the old immediate `engine/renderer/Renderer.hpp` (rewritten in Task 12).

---

### Task 1: Core aliases + Catch2 test harness

**Files:**
- Create: `engine/core/Base.hpp`
- Modify: `vcpkg.json`
- Modify: `CMakeLists.txt`
- Test: `tests/test_sanity.cpp`

**Interfaces:**
- Produces: `engine::Ref<T> = std::shared_ptr<T>`, `engine::Scope<T> = std::unique_ptr<T>`, `Ref<T> CreateRef<T>(Args&&...)`, `Scope<T> CreateScope<T>(Args&&...)`. A `bot_arena_tests` CTest target that compiles `tests/*.cpp` with Catch2 and repo-root include.

- [ ] **Step 1: Write `engine/core/Base.hpp`**

```cpp
#ifndef ENGINE_CORE_BASE_HPP
#define ENGINE_CORE_BASE_HPP

#include <memory>
#include <utility>

namespace engine {

template <typename T>
using Scope = std::unique_ptr<T>;

template <typename T, typename... Args>
Scope<T> CreateScope(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
Ref<T> CreateRef(Args&&... args) {
  return std::make_shared<T>(std::forward<Args>(args)...);
}

}  // namespace engine

#endif  // ENGINE_CORE_BASE_HPP
```

- [ ] **Step 2: Add Catch2 to `vcpkg.json`**

Add `"catch2"` to the `dependencies` array (after `"stb"`).

- [ ] **Step 3: Write the sanity test `tests/test_sanity.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "engine/core/Base.hpp"

TEST_CASE("CreateRef makes a shared_ptr holding the value", "[base]") {
  engine::Ref<int> r = engine::CreateRef<int>(42);
  REQUIRE(r != nullptr);
  REQUIRE(*r == 42);
}
```

- [ ] **Step 4: Add the test target to `CMakeLists.txt`**

After `find_package(Stb REQUIRED)` add:

```cmake
find_package(Catch2 3 CONFIG REQUIRED)
```

At the end of the file add:

```cmake
enable_testing()

add_executable(bot_arena_tests
    tests/test_sanity.cpp
)

target_include_directories(bot_arena_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bot_arena_tests
    PRIVATE
        glm::glm
        Catch2::Catch2WithMain
)

add_test(NAME bot_arena_tests COMMAND bot_arena_tests)
```

- [ ] **Step 5: Configure, build tests, verify they pass**

Run: `cmake --build build 2>&1 | tail -5 && ctest --test-dir build --output-on-failure`
Expected: build succeeds; `bot_arena_tests` passes (1 assertion). If CMake needs reconfigure, run `cmake -S . -B build` first (vcpkg installs Catch2).

- [ ] **Step 6: Commit**

```bash
git add engine/core/Base.hpp vcpkg.json CMakeLists.txt tests/test_sanity.cpp
git commit -m "chore(test): add Ref/Scope core aliases and Catch2 harness"
```

---

### Task 2: Buffer layout (TDD, header-only logic)

**Files:**
- Create: `engine/renderer/Buffer.hpp`
- Test: `tests/test_buffer_layout.cpp`
- Modify: `CMakeLists.txt` (add test source)

**Interfaces:**
- Consumes: `engine::Ref` (Task 1).
- Produces: `enum class ShaderDataType`; `uint32_t ShaderDataTypeSize(ShaderDataType)`; `struct BufferElement { std::string name; ShaderDataType type; uint32_t size; uint32_t offset; bool normalized; uint32_t componentCount() const; }`; `class BufferLayout` with `BufferLayout(std::initializer_list<BufferElement>)`, `uint32_t stride() const`, `const std::vector<BufferElement>& elements() const`, `begin()/end()`; abstract `class VertexBuffer` (Create signatures) and `class IndexBuffer` (Create signature). Factory bodies are added in Task 4.

- [ ] **Step 1: Write the failing test `tests/test_buffer_layout.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/Buffer.hpp"

using engine::BufferLayout;
using engine::ShaderDataType;
using engine::ShaderDataTypeSize;

TEST_CASE("ShaderDataTypeSize returns byte sizes", "[buffer]") {
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Float) == 4);
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Float3) == 12);
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Float4) == 16);
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Mat4) == 64);
  REQUIRE(ShaderDataTypeSize(ShaderDataType::Int2) == 8);
}

TEST_CASE("BufferLayout computes offsets and stride", "[buffer]") {
  BufferLayout layout = {
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float2, "a_uv"},
      {ShaderDataType::Float4, "a_color"},
  };

  const auto& e = layout.elements();
  REQUIRE(e.size() == 3);
  REQUIRE(e[0].offset == 0);
  REQUIRE(e[1].offset == 12);
  REQUIRE(e[2].offset == 20);
  REQUIRE(e[0].componentCount() == 3);
  REQUIRE(e[1].componentCount() == 2);
  REQUIRE(layout.stride() == 36);
}
```

- [ ] **Step 2: Add the test source to `CMakeLists.txt`**

In the `bot_arena_tests` sources list add `tests/test_buffer_layout.cpp`.

- [ ] **Step 3: Run to verify it fails**

Run: `cmake --build build 2>&1 | tail -20`
Expected: FAIL — `engine/renderer/Buffer.hpp` not found / `BufferLayout` undefined.

- [ ] **Step 4: Write `engine/renderer/Buffer.hpp`**

```cpp
#ifndef ENGINE_RENDERER_BUFFER_HPP
#define ENGINE_RENDERER_BUFFER_HPP

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "engine/core/Base.hpp"

namespace engine {

enum class ShaderDataType {
  None = 0,
  Float, Float2, Float3, Float4,
  Mat3, Mat4,
  Int, Int2, Int3, Int4,
  Bool
};

inline uint32_t ShaderDataTypeSize(ShaderDataType type) {
  switch (type) {
    case ShaderDataType::Float:  return 4;
    case ShaderDataType::Float2: return 4 * 2;
    case ShaderDataType::Float3: return 4 * 3;
    case ShaderDataType::Float4: return 4 * 4;
    case ShaderDataType::Mat3:   return 4 * 3 * 3;
    case ShaderDataType::Mat4:   return 4 * 4 * 4;
    case ShaderDataType::Int:    return 4;
    case ShaderDataType::Int2:   return 4 * 2;
    case ShaderDataType::Int3:   return 4 * 3;
    case ShaderDataType::Int4:   return 4 * 4;
    case ShaderDataType::Bool:   return 1;
    case ShaderDataType::None:   return 0;
  }
  return 0;
}

struct BufferElement {
  std::string name;
  ShaderDataType type = ShaderDataType::None;
  uint32_t size = 0;
  uint32_t offset = 0;
  bool normalized = false;

  BufferElement() = default;
  BufferElement(ShaderDataType t, const std::string& n, bool norm = false)
      : name(n), type(t), size(ShaderDataTypeSize(t)), offset(0),
        normalized(norm) {}

  uint32_t componentCount() const {
    switch (type) {
      case ShaderDataType::Float:  return 1;
      case ShaderDataType::Float2: return 2;
      case ShaderDataType::Float3: return 3;
      case ShaderDataType::Float4: return 4;
      case ShaderDataType::Mat3:   return 3 * 3;
      case ShaderDataType::Mat4:   return 4 * 4;
      case ShaderDataType::Int:    return 1;
      case ShaderDataType::Int2:   return 2;
      case ShaderDataType::Int3:   return 3;
      case ShaderDataType::Int4:   return 4;
      case ShaderDataType::Bool:   return 1;
      case ShaderDataType::None:   return 0;
    }
    return 0;
  }
};

class BufferLayout {
 public:
  BufferLayout() = default;
  BufferLayout(std::initializer_list<BufferElement> elements)
      : m_elements(elements) {
    computeOffsetsAndStride();
  }

  uint32_t stride() const { return m_stride; }
  const std::vector<BufferElement>& elements() const { return m_elements; }

  std::vector<BufferElement>::iterator begin() { return m_elements.begin(); }
  std::vector<BufferElement>::iterator end() { return m_elements.end(); }
  std::vector<BufferElement>::const_iterator begin() const {
    return m_elements.begin();
  }
  std::vector<BufferElement>::const_iterator end() const {
    return m_elements.end();
  }

 private:
  void computeOffsetsAndStride() {
    uint32_t offset = 0;
    m_stride = 0;
    for (auto& element : m_elements) {
      element.offset = offset;
      offset += element.size;
      m_stride += element.size;
    }
  }

  std::vector<BufferElement> m_elements;
  uint32_t m_stride = 0;
};

class VertexBuffer {
 public:
  virtual ~VertexBuffer() = default;
  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void setLayout(const BufferLayout& layout) = 0;
  virtual const BufferLayout& layout() const = 0;

  static Ref<VertexBuffer> Create(uint32_t size);
  static Ref<VertexBuffer> Create(const void* data, uint32_t size);
};

class IndexBuffer {
 public:
  virtual ~IndexBuffer() = default;
  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual uint32_t count() const = 0;

  static Ref<IndexBuffer> Create(const uint32_t* indices, uint32_t count);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_BUFFER_HPP
```

- [ ] **Step 5: Run to verify it passes**

Run: `cmake --build build 2>&1 | tail -5 && ctest --test-dir build --output-on-failure`
Expected: PASS (buffer layout tests green).

- [ ] **Step 6: Commit**

```bash
git add engine/renderer/Buffer.hpp tests/test_buffer_layout.cpp CMakeLists.txt
git commit -m "feat(renderer): add ShaderDataType and BufferLayout with tests"
```

---

### Task 3: RenderingAPI interface + OpenGL executor

**Files:**
- Create: `engine/renderer/RenderingAPI.hpp`
- Create: `engine/renderer/RenderingAPI.cpp`
- Create: `engine/renderer/VertexArray.hpp`
- Create: `engine/renderer/opengl/OpenGLRenderingAPI.hpp`
- Create: `engine/renderer/opengl/OpenGLRenderingAPI.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Ref`, `Scope`, `Buffer.hpp`.
- Produces: `class VertexArray` (abstract, see Task 5 fills OpenGL impl — here only the forward-usable base is declared with `Create()` declared, body in Task 5). `class RenderingAPI` with `enum class API { None, OpenGL }`, `static API api()`, `static void setAPI(API)`, `static Scope<RenderingAPI> Create()`, and virtuals `init()`, `setViewport(int,int,int,int)`, `setClearColor(const glm::vec4&)`, `clear()`, `setDepthTest(bool)`, `setBlend(bool)`, `drawIndexed(const Ref<VertexArray>&, uint32_t=0)`, `drawLines(const Ref<VertexArray>&, uint32_t)`, `readPixels(int,int,int,int,void*)`.

- [ ] **Step 1: Write `engine/renderer/VertexArray.hpp`**

```cpp
#ifndef ENGINE_RENDERER_VERTEXARRAY_HPP
#define ENGINE_RENDERER_VERTEXARRAY_HPP

#include "engine/core/Base.hpp"
#include "engine/renderer/Buffer.hpp"

namespace engine {

class VertexArray {
 public:
  virtual ~VertexArray() = default;
  virtual void bind() const = 0;
  virtual void unbind() const = 0;
  virtual void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
  virtual void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;
  virtual const Ref<IndexBuffer>& indexBuffer() const = 0;

  static Ref<VertexArray> Create();
};

}  // namespace engine

#endif  // ENGINE_RENDERER_VERTEXARRAY_HPP
```

- [ ] **Step 2: Write `engine/renderer/RenderingAPI.hpp`**

```cpp
#ifndef ENGINE_RENDERER_RENDERINGAPI_HPP
#define ENGINE_RENDERER_RENDERINGAPI_HPP

#include <cstdint>
#include <glm/glm.hpp>

#include "engine/core/Base.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

class RenderingAPI {
 public:
  enum class API { None = 0, OpenGL = 1 };

  virtual ~RenderingAPI() = default;

  virtual void init() = 0;
  virtual void setViewport(int x, int y, int width, int height) = 0;
  virtual void setClearColor(const glm::vec4& color) = 0;
  virtual void clear() = 0;
  virtual void setDepthTest(bool enabled) = 0;
  virtual void setBlend(bool enabled) = 0;
  virtual void drawIndexed(const Ref<VertexArray>& vertexArray,
                           uint32_t indexCount = 0) = 0;
  virtual void drawLines(const Ref<VertexArray>& vertexArray,
                         uint32_t vertexCount) = 0;
  virtual void readPixels(int x, int y, int width, int height, void* out) = 0;

  static API api() { return s_api; }
  static void setAPI(API api) { s_api = api; }
  static Scope<RenderingAPI> Create();

 private:
  static API s_api;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERINGAPI_HPP
```

- [ ] **Step 3: Write `engine/renderer/RenderingAPI.cpp`**

```cpp
#include "engine/renderer/RenderingAPI.hpp"

#include <stdexcept>

#include "engine/renderer/opengl/OpenGLRenderingAPI.hpp"

namespace engine {

RenderingAPI::API RenderingAPI::s_api = RenderingAPI::API::OpenGL;

Scope<RenderingAPI> RenderingAPI::Create() {
  switch (s_api) {
    case API::OpenGL:
      return CreateScope<OpenGLRenderingAPI>();
    case API::None:
      break;
  }
  throw std::runtime_error("RenderingAPI::Create: unsupported API");
}

}  // namespace engine
```

- [ ] **Step 4: Write `engine/renderer/opengl/OpenGLRenderingAPI.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLRENDERINGAPI_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLRENDERINGAPI_HPP

#include "engine/renderer/RenderingAPI.hpp"

namespace engine {

class OpenGLRenderingAPI final : public RenderingAPI {
 public:
  void init() override;
  void setViewport(int x, int y, int width, int height) override;
  void setClearColor(const glm::vec4& color) override;
  void clear() override;
  void setDepthTest(bool enabled) override;
  void setBlend(bool enabled) override;
  void drawIndexed(const Ref<VertexArray>& vertexArray,
                   uint32_t indexCount = 0) override;
  void drawLines(const Ref<VertexArray>& vertexArray,
                 uint32_t vertexCount) override;
  void readPixels(int x, int y, int width, int height, void* out) override;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLRENDERINGAPI_HPP
```

- [ ] **Step 5: Write `engine/renderer/opengl/OpenGLRenderingAPI.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLRenderingAPI.hpp"

#include <glad/glad.h>

namespace engine {

void OpenGLRenderingAPI::init() {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LINE_SMOOTH);
}

void OpenGLRenderingAPI::setViewport(int x, int y, int width, int height) {
  glViewport(x, y, width, height);
}

void OpenGLRenderingAPI::setClearColor(const glm::vec4& color) {
  glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRenderingAPI::clear() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderingAPI::setDepthTest(bool enabled) {
  if (enabled) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}

void OpenGLRenderingAPI::setBlend(bool enabled) {
  if (enabled) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  } else {
    glDisable(GL_BLEND);
  }
}

void OpenGLRenderingAPI::drawIndexed(const Ref<VertexArray>& vertexArray,
                                     uint32_t indexCount) {
  vertexArray->bind();
  const uint32_t count =
      indexCount ? indexCount : vertexArray->indexBuffer()->count();
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT,
                 nullptr);
}

void OpenGLRenderingAPI::drawLines(const Ref<VertexArray>& vertexArray,
                                   uint32_t vertexCount) {
  vertexArray->bind();
  glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));
}

void OpenGLRenderingAPI::readPixels(int x, int y, int width, int height,
                                    void* out) {
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, out);
}

}  // namespace engine
```

- [ ] **Step 6: Add sources to `CMakeLists.txt`**

In the `bot_arena` sources list add:
```
    engine/renderer/RenderingAPI.cpp
    engine/renderer/opengl/OpenGLRenderingAPI.cpp
```

- [ ] **Step 7: Build to verify it compiles**

Run: `cmake --build build 2>&1 | tail -10`
Expected: build succeeds (new files compile; app still uses the old renderer). `VertexArray::Create` is declared but unreferenced, so no link error.

- [ ] **Step 8: Commit**

```bash
git add engine/renderer/RenderingAPI.hpp engine/renderer/RenderingAPI.cpp \
        engine/renderer/VertexArray.hpp \
        engine/renderer/opengl/OpenGLRenderingAPI.hpp \
        engine/renderer/opengl/OpenGLRenderingAPI.cpp CMakeLists.txt
git commit -m "feat(renderer): add RenderingAPI abstraction and OpenGL executor"
```

---

### Task 4: OpenGL vertex/index buffers

**Files:**
- Create: `engine/renderer/opengl/OpenGLBuffer.hpp`
- Create: `engine/renderer/opengl/OpenGLBuffer.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `VertexBuffer`, `IndexBuffer`, `BufferLayout` (Task 2).
- Produces: `OpenGLVertexBuffer`, `OpenGLIndexBuffer`, and the definitions of `VertexBuffer::Create(...)` / `IndexBuffer::Create(...)`. `OpenGLVertexBuffer::rendererID()` and `OpenGLIndexBuffer::rendererID()` return the GL handle for the VAO wiring in Task 5.

- [ ] **Step 1: Write `engine/renderer/opengl/OpenGLBuffer.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLBUFFER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLBUFFER_HPP

#include "engine/renderer/Buffer.hpp"

namespace engine {

class OpenGLVertexBuffer final : public VertexBuffer {
 public:
  explicit OpenGLVertexBuffer(uint32_t size);
  OpenGLVertexBuffer(const void* data, uint32_t size);
  ~OpenGLVertexBuffer() override;

  void bind() const override;
  void unbind() const override;
  void setData(const void* data, uint32_t size) override;
  void setLayout(const BufferLayout& layout) override { m_layout = layout; }
  const BufferLayout& layout() const override { return m_layout; }

  uint32_t rendererID() const { return m_rendererID; }

 private:
  uint32_t m_rendererID = 0;
  BufferLayout m_layout;
};

class OpenGLIndexBuffer final : public IndexBuffer {
 public:
  OpenGLIndexBuffer(const uint32_t* indices, uint32_t count);
  ~OpenGLIndexBuffer() override;

  void bind() const override;
  void unbind() const override;
  uint32_t count() const override { return m_count; }

  uint32_t rendererID() const { return m_rendererID; }

 private:
  uint32_t m_rendererID = 0;
  uint32_t m_count = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLBUFFER_HPP
```

- [ ] **Step 2: Write `engine/renderer/opengl/OpenGLBuffer.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLBuffer.hpp"

#include <glad/glad.h>

namespace engine {

// --- VertexBuffer factories ---

Ref<VertexBuffer> VertexBuffer::Create(uint32_t size) {
  return CreateRef<OpenGLVertexBuffer>(size);
}

Ref<VertexBuffer> VertexBuffer::Create(const void* data, uint32_t size) {
  return CreateRef<OpenGLVertexBuffer>(data, size);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(const void* data, uint32_t size) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, size, data, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() {
  glDeleteBuffers(1, &m_rendererID);
}

void OpenGLVertexBuffer::bind() const {
  glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
}

void OpenGLVertexBuffer::unbind() const {
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLVertexBuffer::setData(const void* data, uint32_t size) {
  glNamedBufferSubData(m_rendererID, 0, size, data);
}

// --- IndexBuffer factory ---

Ref<IndexBuffer> IndexBuffer::Create(const uint32_t* indices, uint32_t count) {
  return CreateRef<OpenGLIndexBuffer>(indices, count);
}

OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32_t* indices, uint32_t count)
    : m_count(count) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, count * sizeof(uint32_t), indices,
                    GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() {
  glDeleteBuffers(1, &m_rendererID);
}

void OpenGLIndexBuffer::bind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
}

void OpenGLIndexBuffer::unbind() const {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace engine
```

- [ ] **Step 3: Add source to `CMakeLists.txt`**

Add `engine/renderer/opengl/OpenGLBuffer.cpp` to `bot_arena` sources.

- [ ] **Step 4: Build to verify it compiles**

Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add engine/renderer/opengl/OpenGLBuffer.hpp \
        engine/renderer/opengl/OpenGLBuffer.cpp CMakeLists.txt
git commit -m "feat(renderer): add OpenGL vertex and index buffers"
```

---

### Task 5: OpenGL vertex array (DSA attribute wiring)

**Files:**
- Create: `engine/renderer/opengl/OpenGLVertexArray.hpp`
- Create: `engine/renderer/opengl/OpenGLVertexArray.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `VertexArray` (Task 3), `OpenGLVertexBuffer`/`OpenGLIndexBuffer::rendererID()` (Task 4), `BufferLayout` (Task 2).
- Produces: `OpenGLVertexArray` and the definition of `VertexArray::Create()`.

- [ ] **Step 1: Write `engine/renderer/opengl/OpenGLVertexArray.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLVERTEXARRAY_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLVERTEXARRAY_HPP

#include <vector>

#include "engine/renderer/VertexArray.hpp"

namespace engine {

class OpenGLVertexArray final : public VertexArray {
 public:
  OpenGLVertexArray();
  ~OpenGLVertexArray() override;

  void bind() const override;
  void unbind() const override;
  void addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
  void setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;
  const Ref<IndexBuffer>& indexBuffer() const override { return m_indexBuffer; }

 private:
  uint32_t m_rendererID = 0;
  uint32_t m_attribIndex = 0;
  uint32_t m_bindingIndex = 0;
  std::vector<Ref<VertexBuffer>> m_vertexBuffers;
  Ref<IndexBuffer> m_indexBuffer;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLVERTEXARRAY_HPP
```

- [ ] **Step 2: Write `engine/renderer/opengl/OpenGLVertexArray.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLVertexArray.hpp"

#include <glad/glad.h>

#include "engine/renderer/opengl/OpenGLBuffer.hpp"

namespace engine {

static GLenum baseType(ShaderDataType type) {
  switch (type) {
    case ShaderDataType::Float:
    case ShaderDataType::Float2:
    case ShaderDataType::Float3:
    case ShaderDataType::Float4:
    case ShaderDataType::Mat3:
    case ShaderDataType::Mat4:
      return GL_FLOAT;
    case ShaderDataType::Int:
    case ShaderDataType::Int2:
    case ShaderDataType::Int3:
    case ShaderDataType::Int4:
      return GL_INT;
    case ShaderDataType::Bool:
      return GL_BOOL;
    case ShaderDataType::None:
      return GL_NONE;
  }
  return GL_NONE;
}

OpenGLVertexArray::OpenGLVertexArray() {
  glCreateVertexArrays(1, &m_rendererID);
}

OpenGLVertexArray::~OpenGLVertexArray() {
  glDeleteVertexArrays(1, &m_rendererID);
}

void OpenGLVertexArray::bind() const { glBindVertexArray(m_rendererID); }

void OpenGLVertexArray::unbind() const { glBindVertexArray(0); }

void OpenGLVertexArray::addVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
  const uint32_t glId =
      static_cast<OpenGLVertexBuffer&>(*vertexBuffer).rendererID();
  const BufferLayout& layout = vertexBuffer->layout();

  const uint32_t binding = m_bindingIndex++;
  glVertexArrayVertexBuffer(m_rendererID, binding, glId, 0,
                            static_cast<GLsizei>(layout.stride()));

  for (const BufferElement& element : layout) {
    glEnableVertexArrayAttrib(m_rendererID, m_attribIndex);
    glVertexArrayAttribFormat(
        m_rendererID, m_attribIndex,
        static_cast<GLint>(element.componentCount()), baseType(element.type),
        element.normalized ? GL_TRUE : GL_FALSE, element.offset);
    glVertexArrayAttribBinding(m_rendererID, m_attribIndex, binding);
    m_attribIndex++;
  }

  m_vertexBuffers.push_back(vertexBuffer);
}

void OpenGLVertexArray::setIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
  const uint32_t glId =
      static_cast<OpenGLIndexBuffer&>(*indexBuffer).rendererID();
  glVertexArrayElementBuffer(m_rendererID, glId);
  m_indexBuffer = indexBuffer;
}

Ref<VertexArray> VertexArray::Create() {
  return CreateRef<OpenGLVertexArray>();
}

}  // namespace engine
```

- [ ] **Step 3: Add source to `CMakeLists.txt`**

Add `engine/renderer/opengl/OpenGLVertexArray.cpp` to `bot_arena` sources.

- [ ] **Step 4: Build to verify it compiles**

Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add engine/renderer/opengl/OpenGLVertexArray.hpp \
        engine/renderer/opengl/OpenGLVertexArray.cpp CMakeLists.txt
git commit -m "feat(renderer): add OpenGL vertex array with DSA attributes"
```

---

### Task 6: Shader (TDD `#type` parser + OpenGL program)

**Files:**
- Create: `engine/renderer/Shader.hpp`
- Create: `engine/renderer/opengl/OpenGLShader.hpp`
- Create: `engine/renderer/opengl/OpenGLShader.cpp`
- Test: `tests/test_shader_parser.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Ref`, glm.
- Produces: `struct ShaderSources { std::string vertex; std::string fragment; }`; `ShaderSources Shader::ParseSources(const std::string& source)` (static, header-only, no GL); abstract `Shader` with `Create(filepath)`, `Create(name, vert, frag)`, `bind/unbind`, `setInt/setIntArray/setFloat/setFloat3/setFloat4/setMat4`, `name()`. `OpenGLShader` implements it.

- [ ] **Step 1: Write the failing test `tests/test_shader_parser.cpp`**

```cpp
#include <catch2/catch_test_macros.hpp>

#include "engine/renderer/Shader.hpp"

using engine::Shader;

TEST_CASE("ParseSources splits #type sections", "[shader]") {
  const std::string src =
      "#type vertex\n"
      "void main() { gl_Position = vec4(0.0); }\n"
      "#type fragment\n"
      "void main() { }\n";

  const engine::ShaderSources out = Shader::ParseSources(src);

  REQUIRE(out.vertex.find("gl_Position") != std::string::npos);
  REQUIRE(out.fragment.find("void main") != std::string::npos);
  REQUIRE(out.vertex.find("#type") == std::string::npos);
  REQUIRE(out.fragment.find("gl_Position") == std::string::npos);
}
```

- [ ] **Step 2: Add the test source to `CMakeLists.txt`**

Add `tests/test_shader_parser.cpp` to `bot_arena_tests` sources.

- [ ] **Step 3: Run to verify it fails**

Run: `cmake --build build 2>&1 | tail -20`
Expected: FAIL — `Shader.hpp` not found / `ParseSources` undefined.

- [ ] **Step 4: Write `engine/renderer/Shader.hpp`**

```cpp
#ifndef ENGINE_RENDERER_SHADER_HPP
#define ENGINE_RENDERER_SHADER_HPP

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <sstream>
#include <string>

#include "engine/core/Base.hpp"

namespace engine {

struct ShaderSources {
  std::string vertex;
  std::string fragment;
};

class Shader {
 public:
  virtual ~Shader() = default;

  virtual void bind() const = 0;
  virtual void unbind() const = 0;

  virtual void setInt(const std::string& name, int value) = 0;
  virtual void setIntArray(const std::string& name, const int* values,
                           uint32_t count) = 0;
  virtual void setFloat(const std::string& name, float value) = 0;
  virtual void setFloat3(const std::string& name, const glm::vec3& value) = 0;
  virtual void setFloat4(const std::string& name, const glm::vec4& value) = 0;
  virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;

  virtual const std::string& name() const = 0;

  static Ref<Shader> Create(const std::string& filepath);
  static Ref<Shader> Create(const std::string& name, const std::string& vertex,
                            const std::string& fragment);

  // Splits a combined ".glsl" source on "#type vertex" / "#type fragment"
  // markers. Backend-agnostic and GL-free so it can be unit tested.
  static ShaderSources ParseSources(const std::string& source) {
    ShaderSources out;
    const std::string token = "#type";
    std::string* current = nullptr;

    std::istringstream stream(source);
    std::string line;
    std::ostringstream vertex;
    std::ostringstream fragment;

    while (std::getline(stream, line)) {
      if (line.rfind(token, 0) == 0) {
        if (line.find("vertex") != std::string::npos) {
          current = &out.vertex;  // marker; real accumulation below
          current = nullptr;
          current = reinterpret_cast<std::string*>(&vertex);
        } else if (line.find("fragment") != std::string::npos) {
          current = reinterpret_cast<std::string*>(&fragment);
        } else {
          current = nullptr;
        }
        continue;
      }
      if (current == reinterpret_cast<std::string*>(&vertex)) {
        vertex << line << '\n';
      } else if (current == reinterpret_cast<std::string*>(&fragment)) {
        fragment << line << '\n';
      }
    }

    out.vertex = vertex.str();
    out.fragment = fragment.str();
    return out;
  }
};

}  // namespace engine

#endif  // ENGINE_RENDERER_SHADER_HPP
```

Note: the `reinterpret_cast` trick above is ugly. Replace the parser body with this clean version (use it verbatim):

```cpp
  static ShaderSources ParseSources(const std::string& source) {
    ShaderSources out;
    const std::string token = "#type";
    enum class Stage { None, Vertex, Fragment } stage = Stage::None;

    std::istringstream stream(source);
    std::string line;
    std::ostringstream vertex;
    std::ostringstream fragment;

    while (std::getline(stream, line)) {
      if (line.rfind(token, 0) == 0) {
        if (line.find("vertex") != std::string::npos) {
          stage = Stage::Vertex;
        } else if (line.find("fragment") != std::string::npos) {
          stage = Stage::Fragment;
        } else {
          stage = Stage::None;
        }
        continue;
      }
      if (stage == Stage::Vertex) {
        vertex << line << '\n';
      } else if (stage == Stage::Fragment) {
        fragment << line << '\n';
      }
    }

    out.vertex = vertex.str();
    out.fragment = fragment.str();
    return out;
  }
```

(Write only the clean version into the file — the first is shown only to be explicitly discarded.)

- [ ] **Step 5: Write `engine/renderer/opengl/OpenGLShader.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLSHADER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLSHADER_HPP

#include <string>
#include <unordered_map>

#include "engine/renderer/Shader.hpp"

namespace engine {

class OpenGLShader final : public Shader {
 public:
  OpenGLShader(const std::string& name, const std::string& vertex,
               const std::string& fragment);
  ~OpenGLShader() override;

  void bind() const override;
  void unbind() const override;

  void setInt(const std::string& name, int value) override;
  void setIntArray(const std::string& name, const int* values,
                   uint32_t count) override;
  void setFloat(const std::string& name, float value) override;
  void setFloat3(const std::string& name, const glm::vec3& value) override;
  void setFloat4(const std::string& name, const glm::vec4& value) override;
  void setMat4(const std::string& name, const glm::mat4& value) override;

  const std::string& name() const override { return m_name; }

 private:
  int uniformLocation(const std::string& name);

  uint32_t m_rendererID = 0;
  std::string m_name;
  std::unordered_map<std::string, int> m_uniformCache;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLSHADER_HPP
```

- [ ] **Step 6: Write `engine/renderer/opengl/OpenGLShader.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLShader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace engine {

static uint32_t compile(GLenum type, const std::string& source) {
  const uint32_t shader = glCreateShader(type);
  const char* src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  int ok = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    throw std::runtime_error(std::string("Shader compile error: ") + log);
  }
  return shader;
}

Ref<Shader> Shader::Create(const std::string& filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Shader::Create: cannot open " + filepath);
  }
  std::stringstream ss;
  ss << file.rdbuf();
  const ShaderSources sources = ParseSources(ss.str());

  // Derive a name from the file stem.
  std::string name = filepath;
  const size_t slash = name.find_last_of("/\\");
  if (slash != std::string::npos) name = name.substr(slash + 1);
  const size_t dot = name.rfind('.');
  if (dot != std::string::npos) name = name.substr(0, dot);

  return CreateRef<OpenGLShader>(name, sources.vertex, sources.fragment);
}

Ref<Shader> Shader::Create(const std::string& name, const std::string& vertex,
                           const std::string& fragment) {
  return CreateRef<OpenGLShader>(name, vertex, fragment);
}

OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertex,
                           const std::string& fragment)
    : m_name(name) {
  const uint32_t vs = compile(GL_VERTEX_SHADER, vertex);
  const uint32_t fs = compile(GL_FRAGMENT_SHADER, fragment);

  m_rendererID = glCreateProgram();
  glAttachShader(m_rendererID, vs);
  glAttachShader(m_rendererID, fs);
  glLinkProgram(m_rendererID);

  int ok = 0;
  glGetProgramiv(m_rendererID, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetProgramInfoLog(m_rendererID, sizeof(log), nullptr, log);
    throw std::runtime_error(std::string("Shader link error: ") + log);
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
}

OpenGLShader::~OpenGLShader() { glDeleteProgram(m_rendererID); }

void OpenGLShader::bind() const { glUseProgram(m_rendererID); }

void OpenGLShader::unbind() const { glUseProgram(0); }

int OpenGLShader::uniformLocation(const std::string& name) {
  auto it = m_uniformCache.find(name);
  if (it != m_uniformCache.end()) return it->second;
  const int location = glGetUniformLocation(m_rendererID, name.c_str());
  m_uniformCache[name] = location;
  return location;
}

void OpenGLShader::setInt(const std::string& name, int value) {
  glUniform1i(uniformLocation(name), value);
}

void OpenGLShader::setIntArray(const std::string& name, const int* values,
                               uint32_t count) {
  glUniform1iv(uniformLocation(name), static_cast<GLsizei>(count), values);
}

void OpenGLShader::setFloat(const std::string& name, float value) {
  glUniform1f(uniformLocation(name), value);
}

void OpenGLShader::setFloat3(const std::string& name, const glm::vec3& value) {
  glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void OpenGLShader::setFloat4(const std::string& name, const glm::vec4& value) {
  glUniform4fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void OpenGLShader::setMat4(const std::string& name, const glm::mat4& value) {
  glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE,
                     glm::value_ptr(value));
}

}  // namespace engine
```

- [ ] **Step 7: Add sources to `CMakeLists.txt`**

Add `engine/renderer/opengl/OpenGLShader.cpp` to `bot_arena` sources.

- [ ] **Step 8: Build + run tests**

Run: `cmake --build build 2>&1 | tail -5 && ctest --test-dir build --output-on-failure`
Expected: build succeeds; the shader parser test passes.

- [ ] **Step 9: Commit**

```bash
git add engine/renderer/Shader.hpp engine/renderer/opengl/OpenGLShader.hpp \
        engine/renderer/opengl/OpenGLShader.cpp tests/test_shader_parser.cpp \
        CMakeLists.txt
git commit -m "feat(renderer): add Shader abstraction with #type parser"
```

---

### Task 7: Texture2D (stb_image + blank)

**Files:**
- Create: `engine/renderer/Texture.hpp`
- Create: `engine/renderer/opengl/OpenGLTexture.hpp`
- Create: `engine/renderer/opengl/OpenGLTexture.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Ref`.
- Produces: `enum class ImageFormat { None, R8, RGB8, RGBA8, RGBA16F, RGBA32F }`; `enum class TextureFilter`, `TextureWrap`; `struct TextureSpecification`; abstract `Texture` (`width/height/rendererID/setData/bind(slot)`) and `Texture2D` with `Create(spec)` and `Create(path)`.

- [ ] **Step 1: Write `engine/renderer/Texture.hpp`**

```cpp
#ifndef ENGINE_RENDERER_TEXTURE_HPP
#define ENGINE_RENDERER_TEXTURE_HPP

#include <cstdint>
#include <string>

#include "engine/core/Base.hpp"

namespace engine {

enum class ImageFormat { None, R8, RGB8, RGBA8, RGBA16F, RGBA32F };
enum class TextureFilter { Nearest, Linear };
enum class TextureWrap { Repeat, ClampToEdge };

struct TextureSpecification {
  uint32_t width = 1;
  uint32_t height = 1;
  ImageFormat format = ImageFormat::RGBA8;
  TextureFilter filter = TextureFilter::Linear;
  TextureWrap wrap = TextureWrap::Repeat;
  bool generateMips = true;
};

class Texture {
 public:
  virtual ~Texture() = default;
  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;
  virtual uint32_t rendererID() const = 0;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void bind(uint32_t slot = 0) const = 0;
};

class Texture2D : public Texture {
 public:
  static Ref<Texture2D> Create(const TextureSpecification& spec);
  static Ref<Texture2D> Create(const std::string& path);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXTURE_HPP
```

- [ ] **Step 2: Write `engine/renderer/opengl/OpenGLTexture.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLTEXTURE_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLTEXTURE_HPP

#include "engine/renderer/Texture.hpp"

namespace engine {

class OpenGLTexture2D final : public Texture2D {
 public:
  explicit OpenGLTexture2D(const TextureSpecification& spec);
  explicit OpenGLTexture2D(const std::string& path);
  ~OpenGLTexture2D() override;

  uint32_t width() const override { return m_width; }
  uint32_t height() const override { return m_height; }
  uint32_t rendererID() const override { return m_rendererID; }
  void setData(const void* data, uint32_t size) override;
  void bind(uint32_t slot = 0) const override;

 private:
  void allocate();

  TextureSpecification m_spec;
  uint32_t m_rendererID = 0;
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  unsigned int m_internalFormat = 0;
  unsigned int m_dataFormat = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLTEXTURE_HPP
```

- [ ] **Step 3: Write `engine/renderer/opengl/OpenGLTexture.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLTexture.hpp"

#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <stdexcept>

namespace engine {

static void glFormats(ImageFormat format, unsigned int& internalFormat,
                      unsigned int& dataFormat) {
  switch (format) {
    case ImageFormat::R8:      internalFormat = GL_R8;      dataFormat = GL_RED;  break;
    case ImageFormat::RGB8:    internalFormat = GL_RGB8;    dataFormat = GL_RGB;  break;
    case ImageFormat::RGBA8:   internalFormat = GL_RGBA8;   dataFormat = GL_RGBA; break;
    case ImageFormat::RGBA16F: internalFormat = GL_RGBA16F; dataFormat = GL_RGBA; break;
    case ImageFormat::RGBA32F: internalFormat = GL_RGBA32F; dataFormat = GL_RGBA; break;
    case ImageFormat::None:    internalFormat = 0;          dataFormat = 0;       break;
  }
}

static GLint glFilter(TextureFilter f) {
  return f == TextureFilter::Nearest ? GL_NEAREST : GL_LINEAR;
}

static GLint glWrap(TextureWrap w) {
  return w == TextureWrap::ClampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT;
}

OpenGLTexture2D::OpenGLTexture2D(const TextureSpecification& spec)
    : m_spec(spec), m_width(spec.width), m_height(spec.height) {
  allocate();
}

OpenGLTexture2D::OpenGLTexture2D(const std::string& path) {
  int w = 0, h = 0, channels = 0;
  stbi_set_flip_vertically_on_load(1);
  stbi_uc* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
  if (!data) {
    throw std::runtime_error("OpenGLTexture2D: failed to load " + path);
  }

  m_width = static_cast<uint32_t>(w);
  m_height = static_cast<uint32_t>(h);
  m_spec.width = m_width;
  m_spec.height = m_height;
  m_spec.format = channels == 4 ? ImageFormat::RGBA8 : ImageFormat::RGB8;

  allocate();
  const uint32_t bpp = channels == 4 ? 4 : 3;
  setData(data, m_width * m_height * bpp);
  stbi_image_free(data);
}

void OpenGLTexture2D::allocate() {
  glFormats(m_spec.format, m_internalFormat, m_dataFormat);

  glCreateTextures(GL_TEXTURE_2D, 1, &m_rendererID);
  glTextureStorage2D(m_rendererID, 1, m_internalFormat,
                     static_cast<GLsizei>(m_width),
                     static_cast<GLsizei>(m_height));

  glTextureParameteri(m_rendererID, GL_TEXTURE_MIN_FILTER,
                      glFilter(m_spec.filter));
  glTextureParameteri(m_rendererID, GL_TEXTURE_MAG_FILTER,
                      glFilter(m_spec.filter));
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_S, glWrap(m_spec.wrap));
  glTextureParameteri(m_rendererID, GL_TEXTURE_WRAP_T, glWrap(m_spec.wrap));
}

OpenGLTexture2D::~OpenGLTexture2D() { glDeleteTextures(1, &m_rendererID); }

void OpenGLTexture2D::setData(const void* data, uint32_t /*size*/) {
  glTextureSubImage2D(m_rendererID, 0, 0, 0, static_cast<GLsizei>(m_width),
                      static_cast<GLsizei>(m_height), m_dataFormat,
                      GL_UNSIGNED_BYTE, data);
  if (m_spec.generateMips) glGenerateTextureMipmap(m_rendererID);
}

void OpenGLTexture2D::bind(uint32_t slot) const {
  glBindTextureUnit(slot, m_rendererID);
}

Ref<Texture2D> Texture2D::Create(const TextureSpecification& spec) {
  return CreateRef<OpenGLTexture2D>(spec);
}

Ref<Texture2D> Texture2D::Create(const std::string& path) {
  return CreateRef<OpenGLTexture2D>(path);
}

}  // namespace engine
```

- [ ] **Step 4: Add source to `CMakeLists.txt`**

Add `engine/renderer/opengl/OpenGLTexture.cpp` to `bot_arena` sources.

- [ ] **Step 5: Build to verify it compiles**

Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds. (stb_image implementation is already provided by the `STB_IMAGE_WRITE_IMPLEMENTATION` TU; note `stb_image.h` needs `STB_IMAGE_IMPLEMENTATION` — see Step 6.)

- [ ] **Step 6: Provide the `stb_image` implementation**

The existing screenshot TU defines `STB_IMAGE_WRITE_IMPLEMENTATION`. `stb_image.h` (read) needs its own implementation macro exactly once. Add a tiny TU `engine/renderer/opengl/StbImageImpl.cpp`:

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
```

Add `engine/renderer/opengl/StbImageImpl.cpp` to `bot_arena` sources, rebuild:
Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds, no duplicate-symbol errors.

- [ ] **Step 7: Commit**

```bash
git add engine/renderer/Texture.hpp engine/renderer/opengl/OpenGLTexture.hpp \
        engine/renderer/opengl/OpenGLTexture.cpp \
        engine/renderer/opengl/StbImageImpl.cpp CMakeLists.txt
git commit -m "feat(renderer): add Texture2D with stb_image loading"
```

---

### Task 8: UniformBuffer

**Files:**
- Create: `engine/renderer/UniformBuffer.hpp`
- Create: `engine/renderer/opengl/OpenGLUniformBuffer.hpp`
- Create: `engine/renderer/opengl/OpenGLUniformBuffer.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: abstract `UniformBuffer` with `static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding)` and `void setData(const void* data, uint32_t size, uint32_t offset = 0)`.

- [ ] **Step 1: Write `engine/renderer/UniformBuffer.hpp`**

```cpp
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
```

- [ ] **Step 2: Write `engine/renderer/opengl/OpenGLUniformBuffer.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLUNIFORMBUFFER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLUNIFORMBUFFER_HPP

#include "engine/renderer/UniformBuffer.hpp"

namespace engine {

class OpenGLUniformBuffer final : public UniformBuffer {
 public:
  OpenGLUniformBuffer(uint32_t size, uint32_t binding);
  ~OpenGLUniformBuffer() override;

  void setData(const void* data, uint32_t size, uint32_t offset = 0) override;

 private:
  uint32_t m_rendererID = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLUNIFORMBUFFER_HPP
```

- [ ] **Step 3: Write `engine/renderer/opengl/OpenGLUniformBuffer.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLUniformBuffer.hpp"

#include <glad/glad.h>

namespace engine {

OpenGLUniformBuffer::OpenGLUniformBuffer(uint32_t size, uint32_t binding) {
  glCreateBuffers(1, &m_rendererID);
  glNamedBufferData(m_rendererID, size, nullptr, GL_DYNAMIC_DRAW);
  glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_rendererID);
}

OpenGLUniformBuffer::~OpenGLUniformBuffer() {
  glDeleteBuffers(1, &m_rendererID);
}

void OpenGLUniformBuffer::setData(const void* data, uint32_t size,
                                  uint32_t offset) {
  glNamedBufferSubData(m_rendererID, offset, size, data);
}

Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding) {
  return CreateRef<OpenGLUniformBuffer>(size, binding);
}

}  // namespace engine
```

- [ ] **Step 4: Add source + build**

Add `engine/renderer/opengl/OpenGLUniformBuffer.cpp` to `bot_arena` sources.
Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add engine/renderer/UniformBuffer.hpp \
        engine/renderer/opengl/OpenGLUniformBuffer.hpp \
        engine/renderer/opengl/OpenGLUniformBuffer.cpp CMakeLists.txt
git commit -m "feat(renderer): add UniformBuffer abstraction"
```

---

### Task 9: FrameBuffer (MRT + HDR)

**Files:**
- Create: `engine/renderer/FrameBuffer.hpp`
- Create: `engine/renderer/opengl/OpenGLFramebuffer.hpp`
- Create: `engine/renderer/opengl/OpenGLFramebuffer.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Produces: `enum class FramebufferTextureFormat { None, RGBA8, RGBA16F, RGBA32F, RED_INTEGER, DEPTH24STENCIL8 }`; `struct FramebufferSpecification { uint32_t width, height, samples; std::vector<FramebufferTextureFormat> attachments; }`; abstract `FrameBuffer` with `Create(spec)`, `bind()`, `unbind()`, `resize(w,h)`, `colorAttachmentID(index=0)`, `depthAttachmentID()`, `readPixel(attachment,x,y)`, `spec()`.

- [ ] **Step 1: Write `engine/renderer/FrameBuffer.hpp`**

```cpp
#ifndef ENGINE_RENDERER_FRAMEBUFFER_HPP
#define ENGINE_RENDERER_FRAMEBUFFER_HPP

#include <cstdint>
#include <vector>

#include "engine/core/Base.hpp"

namespace engine {

enum class FramebufferTextureFormat {
  None = 0,
  RGBA8,
  RGBA16F,
  RGBA32F,
  RED_INTEGER,
  DEPTH24STENCIL8
};

struct FramebufferSpecification {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t samples = 1;
  std::vector<FramebufferTextureFormat> attachments;
};

class FrameBuffer {
 public:
  virtual ~FrameBuffer() = default;

  virtual void bind() = 0;
  virtual void unbind() = 0;
  virtual void resize(uint32_t width, uint32_t height) = 0;

  virtual uint32_t colorAttachmentID(uint32_t index = 0) const = 0;
  virtual uint32_t depthAttachmentID() const = 0;
  virtual int readPixel(uint32_t attachmentIndex, int x, int y) = 0;

  virtual const FramebufferSpecification& spec() const = 0;

  static Ref<FrameBuffer> Create(const FramebufferSpecification& spec);
};

}  // namespace engine

#endif  // ENGINE_RENDERER_FRAMEBUFFER_HPP
```

- [ ] **Step 2: Write `engine/renderer/opengl/OpenGLFramebuffer.hpp`**

```cpp
#ifndef ENGINE_RENDERER_OPENGL_OPENGLFRAMEBUFFER_HPP
#define ENGINE_RENDERER_OPENGL_OPENGLFRAMEBUFFER_HPP

#include <vector>

#include "engine/renderer/FrameBuffer.hpp"

namespace engine {

class OpenGLFramebuffer final : public FrameBuffer {
 public:
  explicit OpenGLFramebuffer(const FramebufferSpecification& spec);
  ~OpenGLFramebuffer() override;

  void bind() override;
  void unbind() override;
  void resize(uint32_t width, uint32_t height) override;

  uint32_t colorAttachmentID(uint32_t index = 0) const override {
    return m_colorAttachments[index];
  }
  uint32_t depthAttachmentID() const override { return m_depthAttachment; }
  int readPixel(uint32_t attachmentIndex, int x, int y) override;

  const FramebufferSpecification& spec() const override { return m_spec; }

 private:
  void invalidate();
  void destroy();

  FramebufferSpecification m_spec;
  uint32_t m_rendererID = 0;
  std::vector<FramebufferTextureFormat> m_colorFormats;
  FramebufferTextureFormat m_depthFormat = FramebufferTextureFormat::None;
  std::vector<uint32_t> m_colorAttachments;
  uint32_t m_depthAttachment = 0;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_OPENGL_OPENGLFRAMEBUFFER_HPP
```

- [ ] **Step 3: Write `engine/renderer/opengl/OpenGLFramebuffer.cpp`**

```cpp
#include "engine/renderer/opengl/OpenGLFramebuffer.hpp"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace engine {

static bool isDepth(FramebufferTextureFormat format) {
  return format == FramebufferTextureFormat::DEPTH24STENCIL8;
}

static GLenum colorInternalFormat(FramebufferTextureFormat format) {
  switch (format) {
    case FramebufferTextureFormat::RGBA8:       return GL_RGBA8;
    case FramebufferTextureFormat::RGBA16F:     return GL_RGBA16F;
    case FramebufferTextureFormat::RGBA32F:     return GL_RGBA32F;
    case FramebufferTextureFormat::RED_INTEGER: return GL_R32I;
    default:                                    return GL_RGBA8;
  }
}

OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
    : m_spec(spec) {
  for (FramebufferTextureFormat format : spec.attachments) {
    if (isDepth(format)) {
      m_depthFormat = format;
    } else {
      m_colorFormats.push_back(format);
    }
  }
  invalidate();
}

OpenGLFramebuffer::~OpenGLFramebuffer() { destroy(); }

void OpenGLFramebuffer::destroy() {
  if (m_rendererID) {
    glDeleteFramebuffers(1, &m_rendererID);
    glDeleteTextures(static_cast<GLsizei>(m_colorAttachments.size()),
                     m_colorAttachments.data());
    if (m_depthAttachment) glDeleteTextures(1, &m_depthAttachment);
    m_rendererID = 0;
    m_colorAttachments.clear();
    m_depthAttachment = 0;
  }
}

void OpenGLFramebuffer::invalidate() {
  destroy();

  glCreateFramebuffers(1, &m_rendererID);

  m_colorAttachments.resize(m_colorFormats.size());
  if (!m_colorFormats.empty()) {
    glCreateTextures(GL_TEXTURE_2D,
                     static_cast<GLsizei>(m_colorAttachments.size()),
                     m_colorAttachments.data());
    for (size_t i = 0; i < m_colorAttachments.size(); ++i) {
      glTextureStorage2D(m_colorAttachments[i], 1,
                         colorInternalFormat(m_colorFormats[i]),
                         static_cast<GLsizei>(m_spec.width),
                         static_cast<GLsizei>(m_spec.height));
      glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MIN_FILTER,
                          GL_LINEAR);
      glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_MAG_FILTER,
                          GL_LINEAR);
      glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_WRAP_S,
                          GL_CLAMP_TO_EDGE);
      glTextureParameteri(m_colorAttachments[i], GL_TEXTURE_WRAP_T,
                          GL_CLAMP_TO_EDGE);
      glNamedFramebufferTexture(m_rendererID,
                                GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
                                m_colorAttachments[i], 0);
    }
  }

  if (m_depthFormat != FramebufferTextureFormat::None) {
    glCreateTextures(GL_TEXTURE_2D, 1, &m_depthAttachment);
    glTextureStorage2D(m_depthAttachment, 1, GL_DEPTH24_STENCIL8,
                       static_cast<GLsizei>(m_spec.width),
                       static_cast<GLsizei>(m_spec.height));
    glNamedFramebufferTexture(m_rendererID, GL_DEPTH_STENCIL_ATTACHMENT,
                              m_depthAttachment, 0);
  }

  if (m_colorAttachments.size() > 1) {
    GLenum buffers[8];
    for (size_t i = 0; i < m_colorAttachments.size() && i < 8; ++i) {
      buffers[i] = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
    }
    glNamedFramebufferDrawBuffers(
        m_rendererID, static_cast<GLsizei>(m_colorAttachments.size()), buffers);
  } else if (m_colorAttachments.empty()) {
    glNamedFramebufferDrawBuffer(m_rendererID, GL_NONE);
  }

  if (glCheckNamedFramebufferStatus(m_rendererID, GL_FRAMEBUFFER) !=
      GL_FRAMEBUFFER_COMPLETE) {
    spdlog::error("Framebuffer incomplete");
  }
}

void OpenGLFramebuffer::bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
  glViewport(0, 0, static_cast<GLsizei>(m_spec.width),
             static_cast<GLsizei>(m_spec.height));
}

void OpenGLFramebuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

void OpenGLFramebuffer::resize(uint32_t width, uint32_t height) {
  if (width == 0 || height == 0) return;
  m_spec.width = width;
  m_spec.height = height;
  invalidate();
}

int OpenGLFramebuffer::readPixel(uint32_t attachmentIndex, int x, int y) {
  glNamedFramebufferReadBuffer(m_rendererID,
                               GL_COLOR_ATTACHMENT0 + attachmentIndex);
  int value = -1;
  glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &value);
  return value;
}

Ref<FrameBuffer> FrameBuffer::Create(const FramebufferSpecification& spec) {
  return CreateRef<OpenGLFramebuffer>(spec);
}

}  // namespace engine
```

- [ ] **Step 4: Add source + build**

Add `engine/renderer/opengl/OpenGLFramebuffer.cpp` to `bot_arena` sources.
Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds.

- [ ] **Step 5: Commit**

```bash
git add engine/renderer/FrameBuffer.hpp \
        engine/renderer/opengl/OpenGLFramebuffer.hpp \
        engine/renderer/opengl/OpenGLFramebuffer.cpp CMakeLists.txt
git commit -m "feat(renderer): add MRT/HDR framebuffer abstraction"
```

---

### Task 10: RenderPass

**Files:**
- Create: `engine/renderer/RenderPass.hpp`
- Create: `engine/renderer/RenderPass.cpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `FrameBuffer` (Task 9), glm.
- Produces: `struct RenderPassSpecification { Ref<FrameBuffer> target; glm::vec4 clearColor; bool clearDepth; std::string name; }`; `class RenderPass` with `static Ref<RenderPass> Create(const RenderPassSpecification&)` and `const RenderPassSpecification& spec() const`.

- [ ] **Step 1: Write `engine/renderer/RenderPass.hpp`**

```cpp
#ifndef ENGINE_RENDERER_RENDERPASS_HPP
#define ENGINE_RENDERER_RENDERPASS_HPP

#include <glm/glm.hpp>
#include <string>

#include "engine/core/Base.hpp"
#include "engine/renderer/FrameBuffer.hpp"

namespace engine {

struct RenderPassSpecification {
  Ref<FrameBuffer> target;  // nullptr == default framebuffer
  glm::vec4 clearColor{0.08f, 0.09f, 0.11f, 1.0f};
  bool clearDepth = true;
  std::string name;
};

class RenderPass {
 public:
  explicit RenderPass(const RenderPassSpecification& spec) : m_spec(spec) {}

  const RenderPassSpecification& spec() const { return m_spec; }

  static Ref<RenderPass> Create(const RenderPassSpecification& spec);

 private:
  RenderPassSpecification m_spec;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERPASS_HPP
```

- [ ] **Step 2: Write `engine/renderer/RenderPass.cpp`**

```cpp
#include "engine/renderer/RenderPass.hpp"

namespace engine {

Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec) {
  return CreateRef<RenderPass>(spec);
}

}  // namespace engine
```

- [ ] **Step 3: Add source + build**

Add `engine/renderer/RenderPass.cpp` to `bot_arena` sources.
Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds.

- [ ] **Step 4: Commit**

```bash
git add engine/renderer/RenderPass.hpp engine/renderer/RenderPass.cpp CMakeLists.txt
git commit -m "feat(renderer): add RenderPass wrapper"
```

---

### Task 11: High-level Renderer (rewrite of the immediate interface)

**Files:**
- Modify (rewrite): `engine/renderer/Renderer.hpp`
- Create: `engine/renderer/Renderer.cpp`
- Create: `engine/core/AssetPath.hpp`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `RenderingAPI`, `Shader`, `VertexArray`, `UniformBuffer`, `RenderPass`, `Camera` (existing).
- Produces: `class Renderer` (instance) with `void init()`, `void shutdown()`, `void beginScene(const Camera&)`, `void beginRenderPass(const Ref<RenderPass>&)`, `void endRenderPass()`, `void submit(const Ref<Shader>&, const Ref<VertexArray>&, const glm::mat4& transform = glm::mat4(1.0f))`, `void endScene()`, `void onResize(uint32_t,uint32_t)`, `void saveScreenshot(const std::string&, int, int)`. Camera matrix is uploaded to a `UniformBuffer` at **binding 0**; the block layout is `mat4 u_viewProjection` (64 bytes). Also `RenderingAPI& api()` accessor for `DebugRenderer`. `assetPath(rel)` returns an absolute path under the assets dir.

- [ ] **Step 1: Write `engine/core/AssetPath.hpp`**

```cpp
#ifndef ENGINE_CORE_ASSETPATH_HPP
#define ENGINE_CORE_ASSETPATH_HPP

#include <string>

#ifndef BOTARENA_ASSET_DIR
#define BOTARENA_ASSET_DIR "assets"
#endif

namespace engine {

inline std::string assetPath(const std::string& relative) {
  return std::string(BOTARENA_ASSET_DIR) + "/" + relative;
}

}  // namespace engine

#endif  // ENGINE_CORE_ASSETPATH_HPP
```

- [ ] **Step 2: Rewrite `engine/renderer/Renderer.hpp`** (replace the whole file)

```cpp
#ifndef ENGINE_RENDERER_RENDERER_HPP
#define ENGINE_RENDERER_RENDERER_HPP

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

#include "engine/core/Base.hpp"
#include "engine/renderer/Camera.hpp"
#include "engine/renderer/RenderPass.hpp"
#include "engine/renderer/RenderingAPI.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/UniformBuffer.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

class Renderer {
 public:
  void init();
  void shutdown();

  void beginScene(const Camera& camera);
  void endScene();

  void beginRenderPass(const Ref<RenderPass>& pass);
  void endRenderPass();

  void submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray,
              const glm::mat4& transform = glm::mat4(1.0f));

  void onResize(uint32_t width, uint32_t height);
  void saveScreenshot(const std::string& path, int width, int height);

  RenderingAPI& api() { return *m_api; }

 private:
  Scope<RenderingAPI> m_api;
  Ref<UniformBuffer> m_cameraUBO;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_RENDERER_HPP
```

- [ ] **Step 3: Write `engine/renderer/Renderer.cpp`**

```cpp
#include "engine/renderer/Renderer.hpp"

#include <spdlog/spdlog.h>
#include <stb_image_write.h>

#include <vector>

namespace engine {

void Renderer::init() {
  m_api = RenderingAPI::Create();
  m_api->init();
  m_cameraUBO = UniformBuffer::Create(sizeof(glm::mat4), 0);
}

void Renderer::shutdown() {
  m_cameraUBO.reset();
  m_api.reset();
}

void Renderer::beginScene(const Camera& camera) {
  const glm::mat4 viewProjection = camera.viewProjection();
  m_cameraUBO->setData(&viewProjection, sizeof(glm::mat4));
}

void Renderer::endScene() {}

void Renderer::beginRenderPass(const Ref<RenderPass>& pass) {
  const RenderPassSpecification& spec = pass->spec();
  if (spec.target) {
    spec.target->bind();
  } else {
    m_api->setViewport(0, 0, m_viewportWidth, m_viewportHeight);
  }
  m_api->setClearColor(spec.clearColor);
  m_api->clear();
}

void Renderer::endRenderPass() {
  // Unbind to the default framebuffer for the next pass / swap.
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::submit(const Ref<Shader>& shader,
                      const Ref<VertexArray>& vertexArray,
                      const glm::mat4& transform) {
  shader->bind();
  shader->setMat4("u_transform", transform);
  m_api->drawIndexed(vertexArray);
}

void Renderer::onResize(uint32_t width, uint32_t height) {
  m_viewportWidth = static_cast<int>(width);
  m_viewportHeight = static_cast<int>(height);
}

void Renderer::saveScreenshot(const std::string& path, int width, int height) {
  std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
  m_api->readPixels(0, 0, width, height, pixels.data());

  stbi_flip_vertically_on_write(1);
  if (stbi_write_png(path.c_str(), width, height, 3, pixels.data(),
                     width * 3)) {
    spdlog::info("Screenshot saved to {}", path);
  } else {
    spdlog::error("Failed to write screenshot to {}", path);
  }
}

}  // namespace engine
```

Note: `endRenderPass` and `beginRenderPass` reference `glBindFramebuffer` and `m_viewportWidth/Height`. Add `#include <glad/glad.h>` at the top of `Renderer.cpp`, and add two private members to `Renderer.hpp`: `int m_viewportWidth = 0; int m_viewportHeight = 0;` (below `m_cameraUBO`). Update Step 2's header to include them before writing.

- [ ] **Step 4: Add members to the header**

Edit `engine/renderer/Renderer.hpp` private section to:

```cpp
 private:
  Scope<RenderingAPI> m_api;
  Ref<UniformBuffer> m_cameraUBO;
  int m_viewportWidth = 0;
  int m_viewportHeight = 0;
```

- [ ] **Step 5: Add `STB_IMAGE_WRITE_IMPLEMENTATION` ownership**

The old `OpenGLRenderer.cpp` (still present) defines `STB_IMAGE_WRITE_IMPLEMENTATION`. `Renderer.cpp` only `#include <stb_image_write.h>` **without** the macro (the old TU still owns it until Task 16 deletes it). After Task 16 removes `OpenGLRenderer.cpp`, move the `#define STB_IMAGE_WRITE_IMPLEMENTATION` into the `StbImageImpl.cpp` TU (see Task 16 Step 4). For now, do not define it in `Renderer.cpp`.

- [ ] **Step 6: Add source to `CMakeLists.txt`, define asset dir**

Add `engine/renderer/Renderer.cpp` to `bot_arena` sources. Add the compile definition to the `bot_arena` target (near `target_include_directories`):

```cmake
target_compile_definitions(bot_arena
    PRIVATE
        BOTARENA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)
```

- [ ] **Step 7: Build**

Run: `cmake --build build 2>&1 | tail -15`
Expected: FAIL to link/compile — the old `OpenGLRenderer` is still referenced by `Application.cpp` and now `Renderer` has a different interface. This is expected; the app is rewired in Tasks 13–16. To keep this task self-contained, only verify that `Renderer.cpp` **compiles** as a TU:

Run: `cmake --build build --target bot_arena 2>&1 | grep -E "Renderer.cpp|error" | head`
Expected: no compile error originating in `Renderer.cpp` (link errors from `Application`/`OpenGLRenderer` mismatch are acceptable and resolved in Task 13).

> Reviewer note: this is the one task that intentionally leaves the app link-broken. Tasks 12–16 restore a green build. If you prefer an always-green tree, execute Tasks 11–16 as a single review batch.

- [ ] **Step 8: Commit**

```bash
git add engine/renderer/Renderer.hpp engine/renderer/Renderer.cpp \
        engine/core/AssetPath.hpp CMakeLists.txt
git commit -m "feat(renderer): rewrite Renderer as high-level RHI submitter"
```

---

### Task 12: DebugRenderer (batched lines)

**Files:**
- Create: `engine/renderer/DebugRenderer.hpp`
- Create: `engine/renderer/DebugRenderer.cpp`
- Create: `assets/shaders/debug_line.glsl`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: `Renderer::api()`, `VertexArray`, `VertexBuffer`, `Shader`, `assetPath`.
- Produces: `class DebugRenderer` with `void init(Renderer&)`, `void drawLine(const glm::vec3&, const glm::vec3&, const glm::vec4&)`, `void drawCube(const glm::vec3& center, const glm::vec3& size, const glm::vec4&)`, `void drawGrid(float halfSize, float spacing, const glm::vec4&)`, `void flush(Renderer&)`. Reads the camera UBO at binding 0.

- [ ] **Step 1: Write `assets/shaders/debug_line.glsl`**

```glsl
#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;

layout(std140, binding = 0) uniform Camera {
    mat4 u_viewProjection;
};

out vec4 v_color;

void main() {
    v_color = a_color;
    gl_Position = u_viewProjection * vec4(a_position, 1.0);
}

#type fragment
#version 460 core

in vec4 v_color;
out vec4 fragColor;

void main() {
    fragColor = v_color;
}
```

- [ ] **Step 2: Write `engine/renderer/DebugRenderer.hpp`**

```cpp
#ifndef ENGINE_RENDERER_DEBUGRENDERER_HPP
#define ENGINE_RENDERER_DEBUGRENDERER_HPP

#include <glm/glm.hpp>
#include <vector>

#include "engine/core/Base.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/VertexArray.hpp"

namespace engine {

class Renderer;

class DebugRenderer {
 public:
  void init(Renderer& renderer);

  void drawLine(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color);
  void drawCube(const glm::vec3& center, const glm::vec3& size,
                const glm::vec4& color);
  void drawGrid(float halfSize, float spacing, const glm::vec4& color);

  void flush(Renderer& renderer);

 private:
  struct LineVertex {
    glm::vec3 position;
    glm::vec4 color;
  };

  std::vector<LineVertex> m_vertices;
  Ref<VertexArray> m_vertexArray;
  Ref<VertexBuffer> m_vertexBuffer;
  Ref<Shader> m_shader;

  static constexpr uint32_t kMaxVertices = 200000;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_DEBUGRENDERER_HPP
```

- [ ] **Step 3: Write `engine/renderer/DebugRenderer.cpp`**

```cpp
#include "engine/renderer/DebugRenderer.hpp"

#include "engine/core/AssetPath.hpp"
#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/Renderer.hpp"

namespace engine {

void DebugRenderer::init(Renderer& /*renderer*/) {
  m_vertexArray = VertexArray::Create();
  m_vertexBuffer = VertexBuffer::Create(kMaxVertices * sizeof(LineVertex));
  m_vertexBuffer->setLayout({
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float4, "a_color"},
  });
  m_vertexArray->addVertexBuffer(m_vertexBuffer);
  m_shader = Shader::Create(assetPath("shaders/debug_line.glsl"));
  m_vertices.reserve(kMaxVertices);
}

void DebugRenderer::drawLine(const glm::vec3& a, const glm::vec3& b,
                             const glm::vec4& color) {
  m_vertices.push_back({a, color});
  m_vertices.push_back({b, color});
}

void DebugRenderer::drawCube(const glm::vec3& center, const glm::vec3& size,
                             const glm::vec4& color) {
  const glm::vec3 h = size * 0.5f;
  const glm::vec3 p[8] = {
      center + glm::vec3{-h.x, -h.y, -h.z}, center + glm::vec3{h.x, -h.y, -h.z},
      center + glm::vec3{h.x, h.y, -h.z},   center + glm::vec3{-h.x, h.y, -h.z},
      center + glm::vec3{-h.x, -h.y, h.z},  center + glm::vec3{h.x, -h.y, h.z},
      center + glm::vec3{h.x, h.y, h.z},    center + glm::vec3{-h.x, h.y, h.z}};
  const int edges[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
                         6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
  for (int i = 0; i < 24; i += 2) {
    drawLine(p[edges[i]], p[edges[i + 1]], color);
  }
}

void DebugRenderer::drawGrid(float halfSize, float spacing,
                             const glm::vec4& color) {
  for (float i = -halfSize; i <= halfSize; i += spacing) {
    drawLine({i, 0.0f, -halfSize}, {i, 0.0f, halfSize}, color);
    drawLine({-halfSize, 0.0f, i}, {halfSize, 0.0f, i}, color);
  }
}

void DebugRenderer::flush(Renderer& renderer) {
  if (m_vertices.empty()) return;

  const uint32_t count = static_cast<uint32_t>(m_vertices.size());
  m_vertexBuffer->setData(m_vertices.data(), count * sizeof(LineVertex));

  m_shader->bind();
  renderer.api().drawLines(m_vertexArray, count);

  m_vertices.clear();
}

}  // namespace engine
```

- [ ] **Step 4: Add source + build (compile-only for this TU)**

Add `engine/renderer/DebugRenderer.cpp` to `bot_arena` sources.
Run: `cmake --build build --target bot_arena 2>&1 | grep -E "DebugRenderer.cpp|error" | head`
Expected: no compile error in `DebugRenderer.cpp` (app link still broken until Task 13).

- [ ] **Step 5: Commit**

```bash
git add engine/renderer/DebugRenderer.hpp engine/renderer/DebugRenderer.cpp \
        assets/shaders/debug_line.glsl CMakeLists.txt
git commit -m "feat(renderer): add batched DebugRenderer for lines"
```

---

### Task 13: Rewire Application + minimal scene (restore green build)

**Files:**
- Modify: `engine/core/Application.cpp`
- Modify: `game/BotArenaGame.hpp`
- Modify: `game/BotArenaGame.cpp`

**Interfaces:**
- Consumes: new `Renderer` (Task 11), `DebugRenderer` (Task 12).
- Produces: an app that builds and renders the ported debug scene (grid/cubes/lines) through the new `Renderer` + `DebugRenderer` into the default framebuffer.

- [ ] **Step 1: Update `Application::run` and init in `engine/core/Application.cpp`**

Replace the constructor's renderer line and the loop. In the constructor, after `m_renderer = std::make_unique<OpenGLRenderer>();`, change to:

```cpp
  m_renderer = std::make_unique<Renderer>();
  m_renderer->init();
```

Remove the `#include "engine/renderer/opengl/OpenGLRenderer.hpp"` and add `#include "engine/renderer/Renderer.hpp"`.

Replace the render section of `run()`:

```cpp
    for (auto& layer : m_layers) {
      layer->onUpdate(dt);
    }

    m_renderer->onResize(m_window->width(), m_window->height());

    for (auto& layer : m_layers) {
      layer->onRender(*m_renderer, m_window->width(), m_window->height());
    }

    if (screenshotPath) {
      m_renderer->saveScreenshot(screenshotPath, m_window->width(),
                                 m_window->height());
      break;
    }

    m_window->swapBuffers();
```

(The old `beginFrame`/`clear`/`endFrame` calls are removed — clearing now happens inside render passes.)

- [ ] **Step 2: Update `game/BotArenaGame.hpp`**

Add the debug renderer member and include. Add near the other renderer includes:

```cpp
#include "engine/renderer/DebugRenderer.hpp"
```

In the private section add:

```cpp
  engine::DebugRenderer m_debug;
  bool m_debugInitialized = false;
```

- [ ] **Step 3: Update `game/BotArenaGame.cpp` `onRender`**

Replace the body of `onRender` with a passless minimal version that clears via a default-target pass and draws debug lines:

```cpp
void BotArenaGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_debugInitialized) {
    m_debug.init(renderer);
    m_debugInitialized = true;
  }

  const float aspect =
      height > 0 ? static_cast<float>(width) / static_cast<float>(height)
                 : 1.0f;
  m_flyController.resize(aspect);
  m_orbitController.resize(aspect);
  m_topDownCamera.setBounds(-10.0f * aspect, 10.0f * aspect, -10.0f, 10.0f,
                            -100.0f, 100.0f);

  const engine::Camera* camera = &m_flyController.camera();
  if (m_cameraMode == CameraMode::Orbit) camera = &m_orbitController.camera();
  if (m_cameraMode == CameraMode::TopDown) camera = &m_topDownCamera;

  renderer.beginScene(*camera);

  auto pass = engine::RenderPass::Create(
      engine::RenderPassSpecification{nullptr,
                                      {0.08f, 0.09f, 0.11f, 1.0f},
                                      true,
                                      "default"});
  renderer.beginRenderPass(pass);

  m_debug.drawGrid(10.0f, 1.0f, {0.25f, 0.25f, 0.25f, 1.0f});
  m_debug.drawCube({0.0f, 0.5f, -5.0f}, {10.0f, 1.0f, 0.25f},
                   {0.7f, 0.7f, 0.7f, 1.0f});
  m_debug.drawCube({0.0f, 0.5f, 5.0f}, {10.0f, 1.0f, 0.25f},
                   {0.7f, 0.7f, 0.7f, 1.0f});
  m_debug.drawCube({-5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f},
                   {0.7f, 0.7f, 0.7f, 1.0f});
  m_debug.drawCube({5.0f, 0.5f, 0.0f}, {0.25f, 1.0f, 10.0f},
                   {0.7f, 0.7f, 0.7f, 1.0f});
  m_debug.drawCube({2.0f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f},
                   {0.2f, 0.6f, 1.0f, 1.0f});
  m_debug.drawCube({-2.0f, 0.5f, -2.0f}, {1.5f, 1.0f, 1.5f},
                   {0.2f, 0.6f, 1.0f, 1.0f});
  m_debug.drawCube(m_botPosition, {0.5f, 1.0f, 0.5f}, {1.0f, 0.2f, 0.2f, 1.0f});
  m_debug.drawLine(m_botPosition, {0.0f, 0.5f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f});
  m_debug.flush(renderer);

  renderer.endRenderPass();
  renderer.endScene();
}
```

Add `#include "engine/renderer/RenderPass.hpp"` to `BotArenaGame.cpp` includes.

- [ ] **Step 4: Delete the old renderer files so linking resolves**

```bash
git rm engine/renderer/opengl/OpenGLRenderer.hpp \
       engine/renderer/opengl/OpenGLRenderer.cpp
```

Remove `engine/renderer/opengl/OpenGLRenderer.cpp` from `bot_arena` sources in `CMakeLists.txt`.

- [ ] **Step 5: Move `STB_IMAGE_WRITE_IMPLEMENTATION` into its own TU**

Edit `engine/renderer/opengl/StbImageImpl.cpp` (created in Task 7) to own both stb implementations:

```cpp
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
```

- [ ] **Step 6: Build + screenshot verify**

Run: `cmake --build build 2>&1 | tail -5`
Expected: build succeeds (green tree restored).

Run: `cd build && BOTARENA_SCREENSHOT=/tmp/rhi_debug.png ./bot_arena; cd ..`
Expected: `Screenshot saved` logged; `/tmp/rhi_debug.png` shows the arena scene (grid, wall cubes, blue obstacles, red bot + yellow line) — the same scene as before, now through the RHI + DebugRenderer.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "refactor(engine): render debug scene through the new RHI stack"
```

---

### Task 14: Textured cube through the RHI

**Files:**
- Modify: `game/BotArenaGame.hpp`
- Modify: `game/BotArenaGame.cpp`
- Create: `assets/shaders/textured.glsl`
- Create: `assets/textures/checker.png` (generated)

**Interfaces:**
- Consumes: `Renderer::submit`, `VertexArray`, `VertexBuffer`, `IndexBuffer`, `Shader`, `Texture2D`.
- Produces: a textured, indexed cube drawn via `Renderer::submit` with a `u_transform` and a bound `Texture2D` at unit 0.

- [ ] **Step 1: Write `assets/shaders/textured.glsl`**

```glsl
#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

layout(std140, binding = 0) uniform Camera {
    mat4 u_viewProjection;
};

uniform mat4 u_transform;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = u_viewProjection * u_transform * vec4(a_position, 1.0);
}

#type fragment
#version 460 core

in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_texture;

void main() {
    fragColor = texture(u_texture, v_uv);
}
```

- [ ] **Step 2: Generate a checker texture**

Create `assets/textures/` and generate an 8x8 checker PNG. Add a one-off script step (run once, then commit the PNG):

```bash
mkdir -p assets/textures
python3 - <<'PY'
import struct, zlib
size = 64
def png(width, height, rgb):
    def chunk(t, d): 
        c = t + d
        return struct.pack('>I', len(d)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for x in range(width):
            raw += bytes(rgb(x, y))
    sig = b'\x89PNG\r\n\x1a\n'
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    idat = zlib.compress(bytes(raw), 9)
    return sig + chunk(b'IHDR', ihdr) + chunk(b'IDAT', idat) + chunk(b'IEND', b'')
def checker(x, y):
    on = ((x // 8) + (y // 8)) % 2 == 0
    return (230, 230, 230) if on else (60, 90, 160)
open('assets/textures/checker.png', 'wb').write(png(size, size, checker))
print("wrote assets/textures/checker.png")
PY
```

Expected: `wrote assets/textures/checker.png`.

- [ ] **Step 3: Add cube members to `game/BotArenaGame.hpp`**

Add includes:

```cpp
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/Texture.hpp"
#include "engine/renderer/VertexArray.hpp"
```

Add private members:

```cpp
  engine::Ref<engine::VertexArray> m_cubeVA;
  engine::Ref<engine::Shader> m_texturedShader;
  engine::Ref<engine::Texture2D> m_cubeTexture;
```

- [ ] **Step 4: Build cube resources on first render, submit it**

In `BotArenaGame.cpp`, add a helper that builds the cube once (call it inside the `if (!m_debugInitialized)` block, before setting the flag). Add `#include "engine/core/AssetPath.hpp"` and `#include "engine/renderer/Buffer.hpp"`.

```cpp
static engine::Ref<engine::VertexArray> makeCube() {
  // position(3), normal(3), uv(2) — 8 floats per vertex, 24 vertices.
  const float v[] = {
      // +X
      1,-1,-1, 1,0,0, 0,0,  1,1,-1, 1,0,0, 1,0,  1,1,1, 1,0,0, 1,1,  1,-1,1, 1,0,0, 0,1,
      // -X
      -1,-1,1, -1,0,0, 0,0, -1,1,1, -1,0,0, 1,0, -1,1,-1, -1,0,0, 1,1, -1,-1,-1, -1,0,0, 0,1,
      // +Y
      -1,1,-1, 0,1,0, 0,0,  1,1,-1, 0,1,0, 1,0,  1,1,1, 0,1,0, 1,1,  -1,1,1, 0,1,0, 0,1,
      // -Y
      -1,-1,1, 0,-1,0, 0,0,  1,-1,1, 0,-1,0, 1,0,  1,-1,-1, 0,-1,0, 1,1,  -1,-1,-1, 0,-1,0, 0,1,
      // +Z
      -1,-1,1, 0,0,1, 0,0,  1,-1,1, 0,0,1, 1,0,  1,1,1, 0,0,1, 1,1,  -1,1,1, 0,0,1, 0,1,
      // -Z
      1,-1,-1, 0,0,-1, 0,0, -1,-1,-1, 0,0,-1, 1,0, -1,1,-1, 0,0,-1, 1,1, 1,1,-1, 0,0,-1, 0,1,
  };
  uint32_t idx[36];
  for (uint32_t face = 0; face < 6; ++face) {
    const uint32_t o = face * 4;
    const uint32_t base = face * 6;
    idx[base + 0] = o + 0; idx[base + 1] = o + 1; idx[base + 2] = o + 2;
    idx[base + 3] = o + 2; idx[base + 4] = o + 3; idx[base + 5] = o + 0;
  }

  auto va = engine::VertexArray::Create();
  auto vb = engine::VertexBuffer::Create(v, sizeof(v));
  vb->setLayout({
      {engine::ShaderDataType::Float3, "a_position"},
      {engine::ShaderDataType::Float3, "a_normal"},
      {engine::ShaderDataType::Float2, "a_uv"},
  });
  va->addVertexBuffer(vb);
  va->setIndexBuffer(engine::IndexBuffer::Create(idx, 36));
  return va;
}
```

In the init block:

```cpp
    m_cubeVA = makeCube();
    m_texturedShader = engine::Shader::Create(
        engine::assetPath("shaders/textured.glsl"));
    m_cubeTexture = engine::Texture2D::Create(
        engine::assetPath("textures/checker.png"));
```

After `renderer.beginRenderPass(pass);` and before the debug draws, submit the cube:

```cpp
  m_cubeTexture->bind(0);
  m_texturedShader->bind();
  m_texturedShader->setInt("u_texture", 0);
  const glm::mat4 cubeXform =
      glm::translate(glm::mat4(1.0f), {0.0f, 1.5f, 0.0f});
  renderer.submit(m_texturedShader, m_cubeVA, cubeXform);
```

Add `#include <glm/gtc/matrix_transform.hpp>` if not present.

- [ ] **Step 5: Build + screenshot verify**

Run: `cmake --build build 2>&1 | tail -5 && cd build && BOTARENA_SCREENSHOT=/tmp/rhi_cube.png ./bot_arena; cd ..`
Expected: PNG shows the arena scene plus a checker-textured cube hovering at y≈1.5.

- [ ] **Step 6: Commit**

```bash
git add game/BotArenaGame.hpp game/BotArenaGame.cpp \
        assets/shaders/textured.glsl assets/textures/checker.png
git commit -m "feat(game): draw a textured cube through Renderer::submit"
```

---

### Task 15: Offscreen HDR pass + composite (definition of done)

**Files:**
- Modify: `game/BotArenaGame.hpp`
- Modify: `game/BotArenaGame.cpp`
- Create: `assets/shaders/composite.glsl`

**Interfaces:**
- Consumes: `FrameBuffer`, `RenderPass`, `Texture2D` (color attachment as sampler), full RHI.
- Produces: scene rendered into an offscreen `RGBA16F`+depth framebuffer via a render pass, then composited to the default framebuffer with a fullscreen triangle sampling the offscreen color attachment.

- [ ] **Step 1: Write `assets/shaders/composite.glsl`**

```glsl
#type vertex
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;

out vec2 v_uv;

void main() {
    v_uv = a_uv;
    gl_Position = vec4(a_position, 1.0);
}

#type fragment
#version 460 core

in vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_scene;

void main() {
    vec3 color = texture(u_scene, v_uv).rgb;
    fragColor = vec4(color, 1.0);
}
```

- [ ] **Step 2: Add members to `game/BotArenaGame.hpp`**

Add includes:

```cpp
#include "engine/renderer/FrameBuffer.hpp"
#include "engine/renderer/RenderPass.hpp"
```

Add private members:

```cpp
  engine::Ref<engine::FrameBuffer> m_sceneFBO;
  engine::Ref<engine::RenderPass> m_scenePass;
  engine::Ref<engine::RenderPass> m_compositePass;
  engine::Ref<engine::VertexArray> m_fullscreenVA;
  engine::Ref<engine::Shader> m_compositeShader;
  uint32_t m_fbWidth = 0;
  uint32_t m_fbHeight = 0;
```

- [ ] **Step 3: Build offscreen resources + fullscreen triangle**

In `BotArenaGame.cpp`, add a fullscreen-triangle helper:

```cpp
static engine::Ref<engine::VertexArray> makeFullscreenTriangle() {
  // Oversized triangle covering the screen; position(3) + uv(2).
  const float v[] = {
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       3.0f, -1.0f, 0.0f, 2.0f, 0.0f,
      -1.0f,  3.0f, 0.0f, 0.0f, 2.0f,
  };
  const uint32_t idx[] = {0, 1, 2};
  auto va = engine::VertexArray::Create();
  auto vb = engine::VertexBuffer::Create(v, sizeof(v));
  vb->setLayout({
      {engine::ShaderDataType::Float3, "a_position"},
      {engine::ShaderDataType::Float2, "a_uv"},
  });
  va->addVertexBuffer(vb);
  va->setIndexBuffer(engine::IndexBuffer::Create(idx, 3));
  return va;
}
```

In the init block, after the cube setup:

```cpp
    m_fullscreenVA = makeFullscreenTriangle();
    m_compositeShader = engine::Shader::Create(
        engine::assetPath("shaders/composite.glsl"));
```

- [ ] **Step 4: Recreate the FBO on size change, render two passes**

Replace the pass section of `onRender` (the single default pass from Task 13/14) with:

```cpp
  const uint32_t w = static_cast<uint32_t>(width);
  const uint32_t h = static_cast<uint32_t>(height);
  if (!m_sceneFBO || w != m_fbWidth || h != m_fbHeight) {
    engine::FramebufferSpecification spec;
    spec.width = w;
    spec.height = h;
    spec.attachments = {engine::FramebufferTextureFormat::RGBA16F,
                        engine::FramebufferTextureFormat::DEPTH24STENCIL8};
    m_sceneFBO = engine::FrameBuffer::Create(spec);
    m_scenePass = engine::RenderPass::Create(engine::RenderPassSpecification{
        m_sceneFBO, {0.08f, 0.09f, 0.11f, 1.0f}, true, "scene"});
    m_compositePass = engine::RenderPass::Create(engine::RenderPassSpecification{
        nullptr, {0.0f, 0.0f, 0.0f, 1.0f}, true, "composite"});
    m_fbWidth = w;
    m_fbHeight = h;
  }

  renderer.beginScene(*camera);

  // --- Scene pass: offscreen HDR ---
  renderer.beginRenderPass(m_scenePass);

  m_cubeTexture->bind(0);
  m_texturedShader->bind();
  m_texturedShader->setInt("u_texture", 0);
  renderer.submit(m_texturedShader, m_cubeVA,
                  glm::translate(glm::mat4(1.0f), {0.0f, 1.5f, 0.0f}));

  m_debug.drawGrid(10.0f, 1.0f, {0.25f, 0.25f, 0.25f, 1.0f});
  // ... (keep the same debug.drawCube / drawLine calls from Task 13) ...
  m_debug.flush(renderer);

  renderer.endRenderPass();

  // --- Composite pass: default framebuffer, sample the scene color ---
  renderer.beginRenderPass(m_compositePass);
  renderer.api().setDepthTest(false);

  m_cubeTexture->bind(0);  // ensure unit 0 free below
  glBindTextureUnit(0, m_sceneFBO->colorAttachmentID(0));
  m_compositeShader->bind();
  m_compositeShader->setInt("u_scene", 0);
  renderer.submit(m_compositeShader, m_fullscreenVA);

  renderer.api().setDepthTest(true);
  renderer.endRenderPass();

  renderer.endScene();
```

Add `#include <glad/glad.h>` to `BotArenaGame.cpp` for `glBindTextureUnit` (or add a `Texture` bind helper — for now include glad).

Keep the full list of `m_debug.drawCube(...)` / `m_debug.drawLine(...)` calls from Task 13 where the comment indicates.

- [ ] **Step 5: Build + screenshot verify (DoD)**

Run: `cmake --build build 2>&1 | tail -5 && cd build && BOTARENA_SCREENSHOT=/tmp/rhi_dod.png ./bot_arena; cd ..`
Expected: PNG shows the full arena scene + textured cube, rendered through the offscreen HDR pass and composited to screen — matching the current scene plus the cube. This is the spec's definition of done.

- [ ] **Step 6: Commit**

```bash
git add game/BotArenaGame.hpp game/BotArenaGame.cpp assets/shaders/composite.glsl
git commit -m "feat(game): render scene via offscreen HDR pass and composite"
```

---

### Task 16: Cleanup, docs, and final verification

**Files:**
- Modify: `docs/camera.md` (unchanged content; verify still accurate) — no edit expected.
- Create: `docs/renderer.md`
- Verify: no references remain to the old renderer.

**Interfaces:** none (documentation + verification).

- [ ] **Step 1: Verify old renderer is gone**

Run: `grep -rn "OpenGLRenderer\|beginFrame\|drawGrid\b" engine game --include=*.hpp --include=*.cpp | grep -v DebugRenderer`
Expected: no matches referencing the removed immediate renderer (DebugRenderer's own methods are fine).

- [ ] **Step 2: Full build + tests + screenshot**

Run: `cmake --build build 2>&1 | tail -5 && ctest --test-dir build --output-on-failure && cd build && BOTARENA_SCREENSHOT=/tmp/rhi_final.png ./bot_arena; cd ..`
Expected: build green, all unit tests pass, screenshot renders the DoD scene.

- [ ] **Step 3: Write `docs/renderer.md`**

Document the RHI layer: the `Ref<>`/`Create()` model, each resource, the `RenderingAPI`/`Renderer`/`RenderPass`/`DebugRenderer` responsibilities, the camera UBO at binding 0, the `#type` shader format, and the deferred-rendering seam (MRT framebuffer + composable passes). Include the render-flow snippet from the spec's section 8 and a note that PBR is the next spec.

- [ ] **Step 4: Commit**

```bash
git add docs/renderer.md
git commit -m "docs(renderer): document the RHI foundation"
```

---

## Self-Review

**Spec coverage:**
- RHI model (Ref<>/Create/API switch) → Tasks 1, 3, and each resource task. ✓
- Buffer/IndexBuffer + BufferLayout → Task 2 (logic, TDD) + Task 4 (GL). ✓
- VertexArray → Tasks 3 (interface) + 5 (GL DSA). ✓
- Shader (#type + from-string) → Task 6 (TDD parser + GL). ✓
- Texture2D (formats, stb) → Task 7. ✓
- UniformBuffer (binding 0) → Task 8, consumed in Task 11. ✓
- FrameBuffer (MRT/HDR) → Task 9. ✓
- RenderPass → Task 10. ✓
- Renderer (beginScene/submit/passes/onResize/saveScreenshot) → Task 11. ✓
- DebugRenderer → Task 12. ✓
- Old design removed → Task 13. ✓
- Demo: textured cube + offscreen pass + composite + debug ported → Tasks 13–15. ✓
- Deferred readiness (MRT framebuffer, composable passes) → Task 9 + section 8 flow in Task 15. ✓
- Testing (BufferLayout, ShaderDataType, #type parser; screenshot behavioral) → Tasks 1, 2, 6, and screenshots in 13/14/15. ✓

**Placeholder scan:** The Task 6 `Shader.hpp` intentionally shows a bad-then-good parser; the step explicitly says write only the clean version. No TBD/TODO remain elsewhere. Task 13 "keep the same debug.draw* calls" repeats the full list in Task 13 Step 3 (source of truth); Task 15 references them with an explicit "keep from Task 13" pointer — acceptable since the full list is shown once and copied.

**Type consistency:** `Renderer::api()` returns `RenderingAPI&` (Task 11), consumed by `DebugRenderer::flush` (Task 12) and Task 15. `colorAttachmentID(index)` signature consistent (Tasks 9, 15). `Shader::ParseSources`/`ShaderSources` names consistent (Task 6, test). `VertexBuffer::Create`/`IndexBuffer::Create`/`VertexArray::Create` signatures consistent across Tasks 2/4/5. Camera UBO is `mat4` (64 bytes) at binding 0 in Task 11 and both scene shaders (Tasks 12, 14).

**Known intentional exception:** Task 11 leaves the app link-broken until Task 13; both tasks flag this and recommend batching Tasks 11–16 if an always-green tree is required.
