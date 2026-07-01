# Renderer v0.5 — Render Pass & Render-to-Texture

v0.5 makes rendering pass-oriented. A `RenderPass` value names a render target and
how to clear it; the backend becomes a set of pass primitives (`beginPass`,
`execute`, `blit`); and the `Renderer` owns the framebuffers and sequences passes.
As a genuine render-to-texture demo, the swarm is rendered a second time from a
top-down camera into its own framebuffer and composited as a picture-in-picture
minimap.

See `docs/renderer-v0.4.md` for the camera UBO and HDR framebuffer this builds on.

## Data Flow: One Frame

```
BotArenaGame::onRender
  │  renderer.setCamera(*mainCamera)
  │  renderer.setMinimapCamera(m_minimapCam)     // fixed square top-down view
  │  serial grid + walls; parallel swarm (unchanged)
  ▼
Renderer::beginFrame(w,h): reset lanes; m_sceneFBO->resize(w,h); store w,h
  (layers submit into per-lane queues)
Renderer::endFrame  (merge + stable_sort once)
  │  // pass 1: minimap (render-to-texture, 512x512)
  │  backend.beginPass(minimapFBO, dark, clearDepth, 512, 512)
  │  backend.execute(merged, minimapCamera)
  │  // pass 2: scene (HDR)
  │  backend.beginPass(sceneFBO, sceneClear, clearDepth, w, h)
  │  backend.execute(merged, mainCamera)
  │  // pass 3: composite (default framebuffer)
  │  backend.beginPass(nullptr, black, noDepth, w, h)
  │  backend.blit(sceneFBO.color,   {-1,-1, 1, 1})      // fullscreen tonemap
  │  backend.blit(minimapFBO.color, minimapRect(w,h))   // corner inset tonemap
  ▼
saveScreenshot -> readPixels(default framebuffer)
```

The one merged command list is executed twice (two cameras, two targets) — no
re-generation. Sort-key depth stays main-camera-relative; the minimap relies on
the depth test (fine, tiny perf only).

## RenderPass

`engine/renderer/RenderPass.hpp` is a plain, backend-agnostic value:

```cpp
struct RenderPass {
  Ref<Framebuffer> target;   // null == default framebuffer
  glm::vec4 clearColor{0.08f, 0.09f, 0.11f, 1.0f};
  bool clearDepth = true;
};
```

The `Renderer` holds three: `m_minimapPass`, `m_scenePass`, `m_compositePass`.
This is the extraction of v0.4's hardcoded two-pass flow into named, composable
passes — the seam shadow maps, mirrors, portals, and debug views build on.

## Backend Pass Primitives

`RenderBackend` replaced `beginFrame`/`endFrame` with:

```cpp
void beginPass(Framebuffer* target, const glm::vec4& clearColor,
               bool clearDepth, int viewportW, int viewportH);   // bind + clear
void execute(entries, camera, arena, registry);                  // draw scene
void blit(uint32_t sourceColorTexture, const glm::vec4& dstRectNDC);  // tonemap quad
void readPixels(...);
```

- `beginPass` binds `target` (or the default framebuffer + viewport) and clears
  colour (and depth when `clearDepth`).
- `execute` is unchanged from v0.4 — it uploads the camera UBO and draws the mesh
  + line passes into whatever target is bound.
- `blit` draws a **tonemapped** (Reinhard + gamma) unit quad mapped to a `u_rect`
  NDC rectangle, sampling `sourceColorTexture`. One primitive serves both the
  fullscreen scene composite and the corner minimap inset.

The backend no longer owns any framebuffer — it keeps the line shader/VAO, the
camera UBO, and the blit shader + a unit quad. **The scene framebuffer moved to
the `Renderer`.**

## Renderer Orchestration

The `Renderer` owns `m_sceneFBO` (HDR, resized to the window) and `m_minimapFBO`
(HDR, fixed 512×512), the three `RenderPass` values, and two `CameraUniforms`
(`m_camera`, `m_minimapCamera`). `setMinimapCamera(const Camera&)` builds the
second camera block. `endFrame` sequences the three passes through the backend
primitives (see the diagram). All GL stays in the backend, so the `Renderer`
remains game-facing and a future Vulkan backend slots in unchanged.

## Minimap Inset

`engine/renderer/MinimapRect.hpp` computes the top-right NDC rectangle:

```cpp
glm::vec4 minimapRect(int screenW, int screenH, float heightFrac, float margin);
```

Height is `heightFrac` of the screen; width is divided by the screen aspect so the
512×512 source stays square on screen. Called with `heightFrac = 0.32`,
`margin = 0.02`.

## Render-to-Texture

The minimap is a real second render target: the swarm is drawn from the game's
dedicated square top-down `OrthographicCamera` into `m_minimapFBO`, then its
colour attachment is sampled by the corner `blit`. This proves `RenderPass` /
`Framebuffer` support multiple targets and cameras. The pass API stays internal to
the `Renderer` for now (no game-facing pass API yet).

## Testing

- `minimapRect` (Catch2, no GL): the inset sits in the top-right within `[-1,1]`
  and stays square on screen (`(x1-x0)*screenW == (y1-y0)*screenH`).
- Behavioral: the `BOTARENA_SCREENSHOT` path shows the tonemapped swarm fullscreen
  plus a square top-down minimap inset in the top-right corner — three passes, two
  cameras, two render targets.

## Next Milestones

- A **game-facing pass API / render graph** (let systems declare their own passes
  and dependencies).
- **MRT G-buffer + deferred lighting**, **shadow maps** (another render-to-texture
  consumer), bloom / exposure, GPU instancing, and a second backend.
