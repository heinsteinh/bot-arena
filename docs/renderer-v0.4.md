# Renderer v0.4 — Uniform Buffer & HDR Framebuffer

v0.4 adds the two RHI resources that unlock post-processing. A shared camera
`UniformBuffer` is uploaded once per frame and read by every shader at binding 0
(replacing per-shader `u_viewProjection`). The scene renders into an offscreen
**HDR** `Framebuffer`, and a fullscreen composite pass tonemaps (Reinhard +
gamma) it to the default framebuffer.

See `docs/renderer-v0.3.md` for the job system and swarm this builds on.

## Data Flow: One Frame

```
BotArenaGame::onRender
  │  renderer.setCamera(*camera)   // builds CameraUniforms
  │  serial grid + walls; parallel swarm (unchanged from v0.3)
  ▼
Renderer::beginFrame(w,h) -> backend.beginFrame
                              m_sceneFBO->resize(w,h); m_sceneFBO->bind();
                              depth on; clear
  (layers submit into per-lane queues)
Renderer::endFrame
  │  merge lanes + stable_sort
  │  backend.execute(merged, cameraUniforms, arena, registry)
  │     -> m_cameraUBO->setData(cameraUniforms)   // once, binding 0
  │     -> draw scene into the bound HDR FBO (shaders read the UBO)
  │  backend.endFrame()
  │     -> m_sceneFBO->unbind(); viewport; depth off; clear
  │     -> bind m_sceneFBO->colorAttachment(); fullscreen triangle
  │        with composite shader (Reinhard + gamma) -> default framebuffer
  ▼
saveScreenshot -> readPixels(default framebuffer)   // composited image
```

Only command generation is unchanged; the offscreen bind and composite live
entirely inside `OpenGLBackend`, so the `Renderer` and game APIs stay
game-facing (a future Vulkan backend does its own offscreen/composite).

## Camera Uniform Buffer

`engine/renderer/CameraUniforms.hpp`:

```cpp
struct CameraUniforms {          // std140, 208 bytes
  glm::mat4 view, projection, viewProjection;
  glm::vec4 cameraPosition;
};
CameraUniforms makeCameraUniforms(const glm::mat4& view, const glm::mat4& projection);
```

`makeCameraUniforms` computes `viewProjection = projection * view` and recovers
`cameraPosition` from the view matrix (`inverse(view)[3]`), so no new `Camera`
API is needed. `cameraPosition` is populated but no v0.4 shader reads it —
reserved for future specular/PBR.

`UniformBuffer` (`engine/renderer/UniformBuffer.hpp`, abstract +
`OpenGLUniformBuffer`) wraps a GL uniform buffer bound with `glBindBufferBase`:

```cpp
static Ref<UniformBuffer> Create(uint32_t size, uint32_t binding);
void setData(const void* data, uint32_t size, uint32_t offset = 0);
```

The backend owns one at binding 0 (`sizeof(CameraUniforms)`) and uploads the
current frame's camera once in `execute`. Both the mesh shader
(`assets/shaders/mesh.glsl`) and the inline line shader declare the matching
`layout(std140, binding = 0) uniform Camera { ... }` block — **no shader takes
`u_viewProjection` as a plain uniform anymore.**

The `Renderer`'s `setViewProjection(mat4)` became `setCamera(const Camera&)`,
which stores `makeCameraUniforms(camera.view(), camera.projection())`.

## HDR Framebuffer

`engine/renderer/Framebuffer.hpp` (abstract + `OpenGLFramebuffer`):

```cpp
struct FramebufferSpec { uint32_t width = 1280, height = 720; bool hdr = true; };
class Framebuffer {
  void bind();       // bind FBO + set viewport
  void unbind();     // bind default framebuffer (0)
  void resize(uint32_t w, uint32_t h);
  uint32_t colorAttachment() const;   // GL texture id for sampling
};
```

`OpenGLFramebuffer` (DSA) allocates an `RGBA16F` color texture (or `RGBA8` when
`hdr == false`) plus a `DEPTH24_STENCIL8` depth-stencil texture, and checks
completeness. `resize` skips when the size is unchanged or zero, otherwise
recreates the attachments — so `beginFrame(w,h)` calls it every frame but only
rebuilds on an actual window-size change.

## Composite / Tonemap

`endFrame` binds the default framebuffer and draws a single fullscreen triangle
(NDC positions `(-1,-1),(3,-1),(-1,3)`) with the composite shader sampling the
scene's HDR color attachment:

```glsl
vec3 c = texture(u_scene, v_uv).rgb;
c = c / (c + vec3(1.0));       // Reinhard tonemap
c = pow(c, vec3(1.0 / 2.2));   // gamma
fragColor = vec4(c, 1.0);
```

Rendering into `RGBA16F` first means colours can exceed 1.0 before the tonemap
maps them back to displayable range — the seam future bloom / exposure / other
post-processing plug into. The composited image is gamma-corrected and softer
than the raw LDR output of v0.3, which is the expected result of the HDR
pipeline.

## Testing

- `makeCameraUniforms` (Catch2, no GL): for an identity view,
  `viewProjection == projection` and `cameraPosition == (0,0,0,1)`; for a
  `glm::lookAt(eye, ...)` view, `viewProjection == projection * view` and
  `cameraPosition.xyz ≈ eye`.
- Behavioral: the `BOTARENA_SCREENSHOT` path renders the swarm through the HDR
  offscreen target and the tonemap composite (fixed 1280×720, which also
  exercises the framebuffer's first-frame allocation).

## Next Milestones

- **v0.5** — extract `RenderPass{target, clear}` from the now-real two-pass flow;
  expose the framebuffer for game render-to-texture.
- **Later** — MRT G-buffer / deferred lighting, bloom + exposure controls, GPU
  instancing, and a second backend (the abstract `UniformBuffer`/`Framebuffer`
  are ready for it).
