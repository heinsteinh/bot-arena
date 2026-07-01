# Renderer v0.2 — Mesh Path, RHI Resources & Sort Keys

v0.2 builds a filled-mesh render path on top of the v0.1 command buffer. Game
code still submits stateless `RenderCommand`s to a `RenderQueue`, but now the key
that orders them encodes **shader**, **material**, and **depth**, and a new
`MeshRenderer` producer emits `Mesh` commands that the backend draws through RHI
resources while skipping redundant GPU-state changes.

See `docs/renderer-v0.1.md` for the command-buffer fundamentals this extends.

## Data Flow: One Frame

```
BotArenaGame::onRender
   │  renderer.setViewProjection(camera.viewProjection())
   │  MeshRenderer mesh(queue, registry, camera);
   │  DebugRenderer debug(queue);
   │  mesh.submit(cubeMesh, wallMat, transform)   // Mesh cmd, layer=Opaque
   │  debug.drawGrid(...); debug.drawLine(...)     // Line cmds
   ▼
RenderQueue::submit → bump-copy POD command into the frame Arena, record
                      {command.sortKey, cmd*}   (producers set the key now)
   ▼   (Renderer::endFrame)
RenderQueue::sort   → std::stable_sort by [layer|shader|material|depth]
   ▼
RenderBackend::execute(entries, viewProjection, arena, registry)
   ▼
OpenGLBackend
   │  Mesh cmds → resolve handles via registry; skip redundant shader/material
   │             binds; set u_viewProjection/u_transform/u_baseColor;
   │             bind VertexArray; glDrawElements
   │  Line cmds → v0.1 dynamic-VBO batch, one glDrawArrays(GL_LINES)
   ▼   next frame: Arena::reset(); RenderQueue::clear()
GPU
```

## The Sort Key

`makeSortKey` (in `engine/renderer/RenderCommand.hpp`) packs a 64-bit key:

```
[ layer:8 | shader:16 | material:16 | depth:24 ]
```

- **layer** dominates — `Grid(0) < Opaque(1) < Debug(2) < UI(3)`.
- **shader** / **material** are `ResourceRegistry` handles. Sorting by them
  groups draws that share a program or a uniform set, so the backend can bind
  each once per run.
- **depth** is a 24-bit front-to-back distance (0 = nearest) for opaque meshes —
  early-Z friendly ordering within a material.

Two changes from v0.1 make this work:

1. **Producers set the key.** `DebugRenderer` and `MeshRenderer` fill
   `command.sortKey` before submitting; `RenderQueue::submit` records it verbatim
   (it no longer synthesizes a key from a sequence counter).
2. **`std::stable_sort`.** Equal keys keep submission order, so determinism no
   longer needs a sequence field in the key — that space became the depth bits.

## Handle-Based Resources

`ResourceRegistry` (`engine/renderer/ResourceRegistry.hpp`) owns the actual GPU
resources and hands out small `uint16` handles:

```cpp
ShaderHandle   registerShader(const Ref<Shader>&);
MeshHandle     registerMesh(const Ref<VertexArray>&);
MaterialHandle registerMaterial(const Material&);   // Material { vec4 baseColor; ShaderHandle shader; }
```

Commands carry handles, not pointers — so a `RenderCommand` stays a trivially
copyable POD safe to bump-copy into the arena, and the handles double as the
stable integers packed into the sort key. Resources live in the registry for the
frame's lifetime; the backend resolves handles at execute time.

## RHI Resources

The mesh path is drawn through backend-abstracted resources (`Ref<>` +
`Create()`, constructing the OpenGL implementation directly):

- **`Buffer.hpp`** — `ShaderDataType`, `BufferLayout` (computes stride/offset),
  and abstract `VertexBuffer`/`IndexBuffer`. `OpenGLBuffer` uses DSA
  (`glCreateBuffers`, `glNamedBufferData`).
- **`VertexArray.hpp`** — abstract VAO; `OpenGLVertexArray` wires attributes from
  a `BufferLayout` with `glVertexArrayAttribFormat` (DSA).
- **`Shader.hpp`** — abstract program with a header-only `ParseSources` that
  splits a `.glsl` file on `#type vertex` / `#type fragment` markers;
  `OpenGLShader` compiles/links and caches uniform locations.

Shaders load from disk via `assetPath("shaders/...")`, resolved against the
`BOTARENA_ASSET_DIR` compile definition (`engine/core/AssetPath.hpp`).

## MeshRenderer & the Backend Mesh Pass

`MeshRenderer` holds the `RenderQueue`, the `ResourceRegistry`, and the active
`Camera`. On `submit(mesh, material, transform)` it reads the material's shader
handle, computes a front-to-back `depth24` from the transform's translation via
`camera.view()` (normalized by `kFarPlane = 100.0f`), builds the sort key, and
queues a `Mesh` command.

`OpenGLBackend::execute` runs two passes:

1. **Mesh pass** — walk the sorted entries; for each `Mesh` command resolve the
   registry handles and only re-bind the shader when the shader handle changes
   (re-uploading `u_viewProjection`), only re-set `u_baseColor` when the material
   changes, then set `u_transform`, bind the mesh VAO, and `glDrawElements`.
   Depth test and back-face culling are on. Because the queue is sorted by
   shader then material, cubes that share the mesh shader collapse to a single
   shader bind.
2. **Line pass** — the retained v0.1 path: batch every `Line` and wireframe
   `Cube` command into one dynamic VBO and `glDrawArrays(GL_LINES)`.

The mesh shader (`assets/shaders/mesh.glsl`) is flat-shaded: `baseColor *
(ambient + max(N·L, 0))` against a single fixed light direction. The engine's
unit-cube mesh (position + normal, 24 vertices / 36 indices) is built and
registered once in `Renderer::initBuiltins`; every filled cube reuses it with a
per-instance transform and a per-color material.

## Scene Mapping

| Element | Rendered as | Layer |
| --- | --- | --- |
| Grid | `Line` (wireframe) | Grid |
| Wall / obstacle / bot cubes | `Mesh` (filled, flat-shaded) | Opaque |
| Bot → origin line | `Line` (wireframe) | Debug |

## Next Milestones

- **v0.3** — a job system with thread-local arenas so command generation
  parallelizes, then merges before the sort.
- **v0.4** — `UniformBuffer`, `Texture`, `Framebuffer`, and `RenderPass` for
  offscreen/HDR rendering (the deferred-ready seam from the RHI-foundation
  design).
