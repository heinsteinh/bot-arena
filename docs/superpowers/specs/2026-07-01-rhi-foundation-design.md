# RHI Foundation — Design Spec

**Date:** 2026-07-01
**Status:** Approved for planning
**Scope:** Render Hardware Interface (RHI) abstraction layer + OpenGL backend.
**Follow-up (out of scope here):** PBR renderer — mesh/model + glTF, materials,
direct lighting, IBL, shadow maps, tonemapping (separate spec).

## 1. Goal & Motivation

Replace the current single-purpose immediate-mode debug renderer with a proper,
backend-abstracted RHI layer. The current `Renderer`/`OpenGLRenderer` hardcodes
one VAO/VBO, one inline color shader, and a `glBufferSubData`-per-call line
uploader. It cannot express textures, materials, multiple render targets, or
offscreen passes.

This foundation is the base for an eventual **deferred PBR renderer**. Two
constraints follow from that destination and shape the design *now*:

1. **Deferred rendering** requires `FrameBuffer` to support **multiple render
   targets (MRT)** and **HDR float formats** (a G-buffer), and `RenderPass` to
   compose freely (geometry → lighting → composite).
2. **PBR** requires HDR color buffers, uniform buffers for camera/lights, and a
   texture abstraction with float formats and configurable sampling.

The RHI layer is designed so that spec 2 (PBR) adds passes and shaders on top
without changing these interfaces.

## 2. Decisions (locked)

| Decision | Choice |
| --- | --- |
| Scope | RHI foundation only; PBR is a later spec |
| Abstractions | Buffer(+IndexBuffer), VertexArray, Shader, Texture, UniformBuffer, FrameBuffer, GraphicsContext, RenderingAPI, RenderPass, Renderer, DebugRenderer |
| Ownership / backend | `Ref<T>` (=`std::shared_ptr`) + static `Create()` factory switching on `RenderingAPI::API` |
| Shader source | Single `.glsl` file with `#type vertex` / `#type fragment` sections, plus a `Create(name, vertSrc, fragSrc)` from-string path for built-ins |
| Renderer form | Instance, owned by `Application`, passed to `Layer::onRender` |
| Debug drawing | Separate `DebugRenderer` object owned by the game layer |
| Old design | Old immediate `Renderer` / `OpenGLRenderer` removed outright |
| Definition of done | Textured indexed cube + ported debug scene rendered into an offscreen HDR FBO via a `RenderPass`, then composited to the default framebuffer and captured via `BOTARENA_SCREENSHOT` |
| Target GL | OpenGL 4.6 core with DSA (already in use), GLAD loader, SDL platform |

## 3. Architecture

```
        Application / Layer
                │  Renderer&  (high-level, per-frame)
                ▼
┌────────────────────────────────────────────────────┐
│  Renderer       beginScene(camera) -> camera UBO     │  concrete,
│                 beginRenderPass(pass) / endRenderPass │  backend-agnostic;
│                 submit(shader, vao, transform)        │  owns RenderingAPI
│                 endScene(); onResize(); saveScreenshot│
│  DebugRenderer  drawLine/Cube/Grid (batched) + flush  │  concrete, on RHI
│  RenderPass     spec: target FBO + clear settings     │  concrete (data)
└───────────────┬──────────────────────────────────────┘
                │  RHI resources: Ref<>, Create() switches on API
                ▼
  RenderingAPI · VertexBuffer/IndexBuffer · VertexArray · Shader
  Texture2D · UniformBuffer · FrameBuffer   ← abstract base + OpenGL* impl
                │
                ▼
        OpenGL 4.6 core (DSA) via GLAD  ·  SdlOpenGLContext (existing)
```

**Abstract base + `OpenGL*` impl** (each exposes `Create()` that switches on the
active API): `RenderingAPI`, `VertexBuffer`, `IndexBuffer`, `VertexArray`,
`Shader`, `Texture2D`, `UniformBuffer`, `FrameBuffer`.

**Concrete, backend-agnostic** (built on the RHI, no per-backend variant):
`Renderer`, `RenderPass`, `DebugRenderer`.

`GraphicsContext` (abstract) + `SdlOpenGLContext` already exist and are kept.

### Backend selection

`engine/core/Base.hpp` provides:

```cpp
template <typename T> using Ref   = std::shared_ptr<T>;
template <typename T> using Scope = std::unique_ptr<T>;
template <typename T, typename... A> Ref<T>   CreateRef(A&&...);
template <typename T, typename... A> Scope<T> CreateScope(A&&...);
```

`RenderingAPI::API` is a single process-wide value (`None`, `OpenGL`), set to
`OpenGL` at startup. Every resource `Create()` reads it and instantiates the
matching `OpenGL*` type.

## 4. Resource interfaces

### 4.1 Buffer.hpp

```cpp
enum class ShaderDataType { None, Float, Float2, Float3, Float4,
                            Mat3, Mat4, Int, Int2, Int3, Int4, Bool };
uint32_t ShaderDataTypeSize(ShaderDataType);       // bytes

struct BufferElement {
  std::string name; ShaderDataType type;
  uint32_t size; uint32_t offset; bool normalized;
  uint32_t componentCount() const;                 // e.g. Float3 -> 3
};

class BufferLayout {                                // computes stride/offsets
  BufferLayout(std::initializer_list<BufferElement>);
  uint32_t stride() const;
  const std::vector<BufferElement>& elements() const;
};

class VertexBuffer {                                // abstract
  static Ref<VertexBuffer> Create(uint32_t size);              // dynamic
  static Ref<VertexBuffer> Create(const void* data, uint32_t size); // static
  virtual void bind() const = 0; virtual void unbind() const = 0;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void setLayout(const BufferLayout&) = 0;
  virtual const BufferLayout& layout() const = 0;
};

class IndexBuffer {                                 // abstract
  static Ref<IndexBuffer> Create(const uint32_t* indices, uint32_t count);
  virtual void bind() const = 0; virtual void unbind() const = 0;
  virtual uint32_t count() const = 0;
};
```

### 4.2 VertexArray.hpp

```cpp
class VertexArray {                                 // abstract
  static Ref<VertexArray> Create();
  virtual void bind() const = 0; virtual void unbind() const = 0;
  virtual void addVertexBuffer(const Ref<VertexBuffer>&) = 0;
  virtual void setIndexBuffer(const Ref<IndexBuffer>&) = 0;
  virtual const Ref<IndexBuffer>& indexBuffer() const = 0;
};
```

OpenGL impl wires attributes from each vertex buffer's `BufferLayout` using DSA
(`glVertexArrayVertexBuffer`, `glVertexArrayAttribFormat`,
`glVertexArrayAttribBinding`).

### 4.3 Shader.hpp

```cpp
class Shader {                                      // abstract
  static Ref<Shader> Create(const std::string& filepath);         // #type split
  static Ref<Shader> Create(const std::string& name,
                            const std::string& vert, const std::string& frag);
  virtual void bind() const = 0; virtual void unbind() const = 0;
  virtual void setInt(const std::string&, int) = 0;
  virtual void setIntArray(const std::string&, const int*, uint32_t) = 0;
  virtual void setFloat(const std::string&, float) = 0;
  virtual void setFloat3(const std::string&, const glm::vec3&) = 0;
  virtual void setFloat4(const std::string&, const glm::vec4&) = 0;
  virtual void setMat4(const std::string&, const glm::mat4&) = 0;
  virtual const std::string& name() const = 0;
};
```

`.glsl` files are split on `#type vertex` / `#type fragment` markers into stage
sources. Uniform locations are cached per name.

### 4.4 Texture.hpp

```cpp
enum class ImageFormat { None, R8, RGB8, RGBA8, RGBA16F, RGBA32F };
enum class TextureFilter { Nearest, Linear };
enum class TextureWrap   { Repeat, ClampToEdge };
struct TextureSpecification {
  uint32_t width = 1, height = 1;
  ImageFormat format = ImageFormat::RGBA8;
  TextureFilter filter = TextureFilter::Linear;
  TextureWrap wrap = TextureWrap::Repeat;
  bool generateMips = true;
};

class Texture {                                     // abstract base
  virtual uint32_t width() const = 0; virtual uint32_t height() const = 0;
  virtual uint32_t rendererID() const = 0;
  virtual void setData(const void* data, uint32_t size) = 0;
  virtual void bind(uint32_t slot = 0) const = 0;
};
class Texture2D : public Texture {
  static Ref<Texture2D> Create(const TextureSpecification&);      // blank
  static Ref<Texture2D> Create(const std::string& path);         // stb_image
};
```

### 4.5 UniformBuffer.hpp

```cpp
class UniformBuffer {                               // abstract
  static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
  virtual void setData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
};
```

Camera block (`mat4 viewProjection`) lives at **binding 0**. OpenGL impl uses
`glCreateBuffers` + `glBindBufferBase(GL_UNIFORM_BUFFER, binding, id)`.

### 4.6 RenderingAPI.hpp

```cpp
class RenderingAPI {                                // abstract, low-level executor
  enum class API { None, OpenGL };
  static API api();                                 // process-wide
  static Scope<RenderingAPI> Create();
  virtual void init() = 0;
  virtual void setViewport(int x, int y, int w, int h) = 0;
  virtual void setClearColor(const glm::vec4&) = 0;
  virtual void clear() = 0;
  virtual void setDepthTest(bool) = 0;
  virtual void setBlend(bool) = 0;
  virtual void drawIndexed(const Ref<VertexArray>&, uint32_t count = 0) = 0;
  virtual void drawLines(const Ref<VertexArray>&, uint32_t vertexCount) = 0;
  virtual void readPixels(int x, int y, int w, int h, void* out) = 0; // screenshot
};
```

## 5. FrameBuffer & RenderPass (deferred-ready)

```cpp
enum class FramebufferTextureFormat { None, RGBA8, RGBA16F, RGBA32F,
                                      RED_INTEGER, DEPTH24STENCIL8 };
struct FramebufferSpecification {
  uint32_t width = 0, height = 0, samples = 1;
  std::vector<FramebufferTextureFormat> attachments;   // MRT: G-buffer
};
class FrameBuffer {                                 // abstract + OpenGLFramebuffer
  static Ref<FrameBuffer> Create(const FramebufferSpecification&);
  virtual void bind() = 0; virtual void unbind() = 0;
  virtual void resize(uint32_t, uint32_t) = 0;
  virtual uint32_t colorAttachmentID(uint32_t index = 0) const = 0;
  virtual uint32_t depthAttachmentID() const = 0;
  virtual int readPixel(uint32_t attachment, int x, int y) = 0;   // picking later
  virtual const FramebufferSpecification& spec() const = 0;
};

struct RenderPassSpecification {
  Ref<FrameBuffer> target;                          // nullptr == default FBO
  glm::vec4 clearColor{0.08f, 0.09f, 0.11f, 1.0f};
  bool clearDepth = true;
  std::string name;
};
class RenderPass {                                  // concrete (data)
  static Ref<RenderPass> Create(const RenderPassSpecification&);
  const RenderPassSpecification& spec() const;
};
```

`Renderer::beginRenderPass(pass)` binds the target FBO (or default), sets the
viewport to its size, applies clear color/flags, and clears. `endRenderPass()`
unbinds to the default framebuffer.

**Deferred seam:** spec 2 adds a geometry pass (MRT G-buffer FBO with
`RGBA16F` position/normal + `RGBA8` albedo + depth), a lighting pass (HDR FBO +
lights UBO), and a composite/tonemap pass — all expressed with the *same*
`FrameBuffer` and `RenderPass` types defined here. No RHI change required.

## 6. Renderer & DebugRenderer

```cpp
class Renderer {                                    // instance, owned by Application
  void init(); void shutdown();
  void beginScene(const Camera&);                   // uploads camera UBO (binding 0)
  void beginRenderPass(const Ref<RenderPass>&); void endRenderPass();
  void submit(const Ref<Shader>&, const Ref<VertexArray>&,
              const glm::mat4& transform = glm::mat4(1.0f));
  void endScene();
  void onResize(uint32_t, uint32_t);
  void saveScreenshot(const std::string&, int w, int h);  // BOTARENA_SCREENSHOT
 private:
  Scope<RenderingAPI> m_api;
  Ref<UniformBuffer> m_cameraUBO;
};
```

`submit` binds the shader, sets the per-object `u_transform`, binds the VAO, and
calls `RenderingAPI::drawIndexed`.

`DebugRenderer` batches `drawLine/drawCube/drawGrid` into one dynamic
`VertexBuffer` (position + color layout) and flushes via
`RenderingAPI::drawLines`, reading the same camera UBO at binding 0. It replaces
today's per-call `glBufferSubData` path and lets the existing grid/cubes/lines
keep rendering.

## 7. Directory layout

```
engine/core/Base.hpp
engine/renderer/RenderingAPI.hpp
engine/renderer/Buffer.hpp
engine/renderer/VertexArray.hpp
engine/renderer/Shader.hpp
engine/renderer/Texture.hpp
engine/renderer/UniformBuffer.hpp
engine/renderer/FrameBuffer.hpp
engine/renderer/RenderPass.hpp
engine/renderer/Renderer.hpp
engine/renderer/DebugRenderer.hpp
engine/renderer/opengl/OpenGLRenderingAPI.{hpp,cpp}
engine/renderer/opengl/OpenGLBuffer.{hpp,cpp}
engine/renderer/opengl/OpenGLVertexArray.{hpp,cpp}
engine/renderer/opengl/OpenGLShader.{hpp,cpp}
engine/renderer/opengl/OpenGLTexture.{hpp,cpp}
engine/renderer/opengl/OpenGLUniformBuffer.{hpp,cpp}
engine/renderer/opengl/OpenGLFramebuffer.{hpp,cpp}
assets/shaders/textured.glsl
assets/shaders/debug_line.glsl
assets/shaders/composite.glsl
```

`Renderer`, `RenderPass`, `DebugRenderer` are `.hpp/.cpp` under
`engine/renderer/`. The old `Renderer.hpp` (immediate interface) and
`opengl/OpenGLRenderer.{hpp,cpp}` are deleted. `Camera.hpp` and its subclasses
are unchanged. `Layer::onRender(Renderer&, int, int)` keeps its shape but now
receives the new `Renderer`.

## 8. Demo / Definition of Done

`BotArenaGame` owns a `DebugRenderer`, a textured cube (`VertexArray` +
`IndexBuffer`), a `Texture2D`, an offscreen HDR `RenderPass`, and a composite
`RenderPass`:

```
Renderer::beginScene(camera)                        // camera UBO
Renderer::beginRenderPass(offscreenPass)            // RGBA16F color + depth FBO
    Renderer::submit(texturedShader, cubeVAO, transform)  // textured indexed cube
    debug.drawGrid(...); debug.drawCube(...); debug.drawLine(...); debug.flush()
Renderer::endRenderPass()
Renderer::beginRenderPass(compositePass)            // default framebuffer
    Renderer::submit(compositeShader, fullscreenTri)      // samples offscreen color
Renderer::endRenderPass()
Renderer::endScene()
```

**Done when:** `BOTARENA_SCREENSHOT=out.png ./bot_arena` produces a PNG showing
the existing arena scene (grid, wall cubes, blue obstacles, red bot + yellow
line) **plus** a textured cube, rendered through the offscreen HDR pass and
composited to screen. The offscreen→composite step is intentionally the same
mechanism a deferred lighting/tonemap pass will use.

## 9. Testing

- **Unit tests (pure logic, no GL context):**
  - `BufferLayout` computes correct per-element offsets and total stride.
  - `ShaderDataTypeSize` / `componentCount` for each type.
  - Shader `#type` parser splits a multi-stage `.glsl` string correctly
    (including whitespace / missing-stage edge cases).
  - A small test target is added (Catch2 via vcpkg preferred; fallback: a
    minimal assert-based harness) — chosen in the implementation plan.
- **Behavioral:** the offscreen screenshot render is the integration check for
  the GL path (GL behavior can't be unit-tested without a context). Compared
  visually against the current scene plus the new textured cube.

## 10. Out of scope (future PBR spec)

Mesh/Model abstraction + glTF loading, `Material` system, MeshGenerator,
Cook-Torrance direct lighting, image-based lighting (IBL) + environment maps,
shadow maps, HDR tonemapping/bloom, the full deferred G-buffer passes, and any
second backend (Vulkan). The interfaces above are designed to accept these
without modification.
```
