# Renderer v0.1 — Stateless Command-Buffer Pipeline

The v0.1 renderer replaces the immediate-mode drawing interface with a stateless,
layered command-buffer architecture. Game code submits `RenderCommand`s to a queue;
the queue is sorted by a 64-bit layer-and-sequence key; a pluggable `RenderBackend`
executes the sorted commands. Nobody outside the backend touches OpenGL.

This is the public rendering API going forward. Each v0.x milestone adds capability
without changing this surface. v0.1 produces the exact same visible scene as the
immediate renderer, verifiable by screenshot regression.

```
Input  ──►  CameraController  ──►  Camera  ──►  Renderer::setViewProjection
           update(dt)                           (uploads u_viewProjection once per frame)

DebugRenderer::drawGrid/drawCube/drawLine  ──►  RenderCommand  ──►  Arena  ──►  RenderQueue
                                                  (POD, bump-copied)         (sorted by key)
                                                                                    │
                                                                                    ▼
                                                                         RenderBackend::execute
                                                                                    │
                                                                                    ▼
                                                                         OpenGLBackend
                                                                                    │
                                                                            (expand to vertices,
                                                                             upload, draw)
                                                                                    │
                                                                                    ▼
                                                                                   GPU
```

## Data Flow: One Frame

```
Renderer::beginFrame(width, height)
  ├─ m_arena.reset()                  // rewind frame allocator to 0
  ├─ m_queue.clear()                  // empty entry list, reset sequence counter
  └─ m_backend->beginFrame(w, h)      // glViewport, glClear, depth state

BotArenaGame::onRender(renderer, w, h)
  ├─ renderer.setViewProjection(camera.viewProjection())  // cache mat4
  ├─ DebugRenderer debug(renderer.queue());
  ├─ debug.drawGrid(...)      ──►  RenderQueue::submit (Grid layer)
  ├─ debug.drawCube(...)      ──►  RenderQueue::submit (Opaque layer)
  └─ debug.drawLine(...)      ──►  RenderQueue::submit (Debug layer)

Renderer::endFrame()
  ├─ m_queue.sort()                   // std::sort by [layer:8][sequence:56] key
  ├─ m_backend->execute(
  │    m_queue.entries(),
  │    m_viewProjection,
  │    m_arena)                       // iterate entries, expand to vertices, render
  └─ m_backend->endFrame()            // glFlush (optional)

window.swapBuffers()
```

## Core Components

### RenderCommand — `engine/renderer/RenderCommand.hpp`

A plain-old-data (POD), trivially-copyable struct holding a single draw operation.
Safe to bump-copy into the frame allocator without invoking constructors.

```cpp
enum class RenderLayer : uint8_t { Grid = 0, Opaque = 1, Debug = 2, UI = 3 };
enum class RenderCommandType : uint8_t { Line, Cube };

struct RenderCommand {
  RenderCommandType type = RenderCommandType::Line;
  RenderLayer layer = RenderLayer::Debug;

  glm::vec3 position{0.0f};    // cube center
  glm::vec3 scale{1.0f};       // cube half-size
  glm::vec4 color{1.0f};       // RGBA (alpha unused in v0.1)

  glm::vec3 lineStart{0.0f};   // line start (for type=Line)
  glm::vec3 lineEnd{0.0f};     // line end   (for type=Line)

  uint64_t sortKey = 0;        // set by RenderQueue::submit
};

// Pack layer (8 bits) and sequence (56 bits) into a single sort key.
// Higher layer value sorts later; within a layer, higher sequence sorts later.
inline uint64_t makeSortKey(RenderLayer layer, uint64_t sequence) {
  return (static_cast<uint64_t>(layer) << 56) | (sequence & 0x00FFFFFFFFFFFFFFull);
}
```

### Arena — `engine/core/Arena.hpp`

A linear/bump allocator over a single heap block, allocated once at startup and
reset each frame. Commands and generated geometry are allocated from it.

```cpp
class Arena {
 public:
  explicit Arena(std::size_t capacityBytes);
  ~Arena();

  void* allocate(std::size_t size, std::size_t alignment = alignof(std::max_align_t));

  template <typename T>
  T* alloc(std::size_t count = 1) {
    return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
  }

  void reset();           // rewind to 0; does not call destructors (POD only)
  std::size_t used() const;
  std::size_t capacity() const;
};
```

- `allocate` aligns the current offset up to the requested alignment, returns the
  pointer, and advances the offset.
- On overflow, throws `std::runtime_error("Arena: out of memory")`.
- `reset()` sets offset to 0, allowing the heap to be reused next frame.

### RenderQueue — `engine/renderer/RenderQueue.hpp`

Owns a reference to the frame `Arena` and accumulates a list of sorted entries.
Each `submit` copies the command into the arena and records its key and pointer.

```cpp
struct RenderEntry {
  uint64_t sortKey;
  const RenderCommand* command;  // points into the frame Arena
};

class RenderQueue {
 public:
  explicit RenderQueue(Arena& arena);

  void submit(const RenderCommand& command);       // copy into arena, record entry
  void clear();                                    // clear entries, reset sequence
  void sort();                                     // std::sort by sortKey
  const std::vector<RenderEntry>& entries() const;

 private:
  Arena& m_arena;
  std::vector<RenderEntry> m_entries;
  uint64_t m_sequence = 0;  // monotonically incremented; used as tiebreak in sort key
};
```

**Submission flow:**
1. `submit(cmd)` allocates a `RenderCommand` in the arena.
2. Copies the argument command into that allocation.
3. Overwrites the command's `sortKey` with `makeSortKey(cmd.layer, m_sequence++)`.
4. Appends `{sortKey, ptr}` to `m_entries`.

This ensures that within a layer, commands sort by submission order (sequence).
Across layers, `Grid < Opaque < Debug < UI` due to layer values in the high byte.

### RenderBackend — `engine/renderer/RenderBackend.hpp`

An abstract interface for rendering backends. Receives sorted entries and renders
them using a concrete implementation (e.g., OpenGL).

```cpp
class RenderBackend {
 public:
  virtual ~RenderBackend() = default;

  // One-time setup per frame (viewport, clear, depth state).
  virtual void beginFrame(int width, int height) = 0;

  // Render sorted commands.
  // scratch: the frame Arena, for building temporary vertex data.
  virtual void execute(const std::vector<RenderEntry>& entries,
                       const glm::mat4& viewProjection,
                       Arena& scratch) = 0;

  // Post-render cleanup (flush, barrier, etc.).
  virtual void endFrame() = 0;

  // Read pixels for screenshots.
  virtual void readPixels(int x, int y, int width, int height, void* out) = 0;

  // Factory (creates OpenGLBackend in v0.1).
  static Scope<RenderBackend> Create();
};
```

### OpenGLBackend — `engine/renderer/opengl/OpenGLBackend.hpp`

The concrete OpenGL implementation. Owns one line shader (3D positions + RGBA
color) and one dynamic VAO/VBO. Expands commands to line vertices in the arena,
then renders all vertices in one draw call.

**Expansion logic:**
- `Cube` → 12 edges, 24 line vertices (2 vertices per edge).
- `Line` → 2 vertices.
- All accumulated into one contiguous array in the `scratch` arena.

**Render loop:**
1. `beginFrame`: `glViewport`, `glEnable(GL_DEPTH_TEST)`, `glClearColor` +
   `glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)`.
2. Iterate entries; expand each command's vertices into `scratch`.
3. Upload the vertex array with `glBufferSubData`.
4. Set `u_viewProjection` uniform.
5. `glDrawArrays(GL_LINES, 0, vertexCount)`.
6. `endFrame`: `glFlush` (optional).

**Screenshot:**
- `readPixels(x, y, w, h, out)`: `glReadPixels(GL_RGB, GL_UNSIGNED_BYTE)` into
  the output buffer. Caller uses `stbi_write_png` to save with vertical flip.

### DebugRenderer — `engine/renderer/DebugRenderer.hpp`

A thin producer that wraps a `RenderQueue` reference and submits `RenderCommand`s
on behalf of game code.

```cpp
class DebugRenderer {
 public:
  explicit DebugRenderer(RenderQueue& queue);

  void drawLine(const glm::vec3& a, const glm::vec3& b, const glm::vec4& color);
  void drawCube(const glm::vec3& center, const glm::vec3& size, const glm::vec4& color);
  void drawGrid(float halfSize, float spacing, const glm::vec4& color);

 private:
  RenderQueue& m_queue;
};
```

- `drawLine(a, b, color)` → `RenderCommand{type=Line, layer=Debug, lineStart=a, lineEnd=b, color}`.
- `drawCube(center, size, color)` → `RenderCommand{type=Cube, layer=Opaque, position=center, scale=size, color}`.
- `drawGrid(halfSize, spacing, color)` → one `Line` command per grid line, `layer=Grid`.

### Renderer — `engine/renderer/Renderer.hpp`

Owns the frame `Arena`, `RenderQueue`, and `RenderBackend`. Orchestrates the frame
lifecycle and caches the view-projection matrix.

```cpp
class Renderer {
 public:
  Renderer();

  void beginFrame(int width, int height);
  void endFrame();
  void setViewProjection(const glm::mat4& viewProjection);
  RenderQueue& queue();                    // for DebugRenderer to submit to
  void saveScreenshot(const std::string& path, int width, int height);

 private:
  Arena m_arena;                           // 8 MiB per-frame allocator
  RenderQueue m_queue;                     // sorted entry list
  Scope<RenderBackend> m_backend;          // OpenGLBackend in v0.1
  glm::mat4 m_viewProjection{1.0f};
  int m_width = 0, m_height = 0;
};
```

**Frame orchestration:**

```cpp
void Renderer::beginFrame(int width, int height) {
  m_width = width;
  m_height = height;
  m_arena.reset();
  m_queue.clear();
  m_backend->beginFrame(width, height);
}

void Renderer::endFrame() {
  m_queue.sort();
  m_backend->execute(m_queue.entries(), m_viewProjection, m_arena);
  m_backend->endFrame();
}

void Renderer::setViewProjection(const glm::mat4& vp) {
  m_viewProjection = vp;  // cached and used once in endFrame
}

void Renderer::saveScreenshot(const std::string& path, int width, int height) {
  std::vector<uint8_t> pixels(width * height * 3);
  m_backend->readPixels(0, 0, width, height, pixels.data());
  // Flip vertically and write PNG...
}
```

## Draw Order & Layer Mapping

The sort key `[layer:8][sequence:56]` produces a deterministic draw order:

| Layer | Enum value | Drawn when |
| ----- | ---------- | ---------- |
| Grid  | 0          | First (background grid) |
| Opaque | 1         | Second (walls, obstacles, robot body) |
| Debug | 2          | Third (line from bot to origin) |
| UI    | 3          | Fourth (reserved for future UI) |

Within each layer, commands draw in submission order (the `sequence` field).
Depth testing is enabled, so occluded geometry is handled correctly.

This matches the current scene: grid drawn first, cubes layered on top, yellow bot
line drawn last, all with depth testing. Identical output, different architecture.

## Usage: Rendering a Frame

```cpp
// In BotArenaGame::onRender(Renderer& renderer, int w, int h)
// 1. Set the camera for this frame
renderer.setViewProjection(activeCamera.viewProjection());

// 2. Get the queue and create a debug producer
DebugRenderer debug(renderer.queue());

// 3. Submit rendering commands
debug.drawGrid(10.0f, 1.0f, glm::vec4{0.3f, 0.3f, 0.3f, 1.0f});
for (const auto& obstacle : obstacles) {
  debug.drawCube(obstacle.pos, obstacle.size, glm::vec4{0.0f, 0.0f, 1.0f, 1.0f});
}
debug.drawLine(robot.pos, {0, 0, 0}, glm::vec4{1.0f, 1.0f, 0.0f, 1.0f});

// endFrame handles queue sort and backend execution
// (called by Application::run after all layers have submitted)
```

## Next Milestones

**v0.2: Real sort keys and filled geometry**
- Implement true depth-based sort keys (depth from camera + material ID).
- Add filled/shaded cube geometry (vertex arrays, shaders).
- Introduce RHI resource abstractions (`VertexArray`, `Shader`, `Texture`) as
  backend building blocks.

**v0.3: Multithreading & thread-local arenas**
- Introduce `JobSystem` for parallel command generation.
- Thread-local `Arena` instances; merge entries at the end of the frame.
- Lock-free command submission within a layer.

**v0.4+: Frame resources & render graph**
- Framebuffer abstraction; render passes (§5.4).
- Render graph as nodes + dependencies.
- Vulkan backend.
