# Engine v0.31 — Camera Views Demo

v0.31 adds a new example game, `camera_demo`, that views a shared 3D scene
through four camera projections and overlays the v0.30 SDF-effects HUD on
top of each — proving the screen-space text system renders legibly no
matter what the 3D camera is doing. It reuses the existing camera
controllers, mesh renderer, and text APIs verbatim; there is no new engine
code. Design rationale:
`docs/superpowers/specs/2026-07-15-engine-v0.31-camera-demo-design.md`.

## What's new (`games/camera_demo/`)

- **`CameraDemoGame`** (`camerademo` namespace, `camera_demo_game` target)
  holds one instance of each camera type — `OrbitCameraController m_orbit`,
  `FlyCameraController m_fly`, `PerspectiveCamera m_front`,
  `OrthographicCamera m_top` — plus an active-view index `m_view` (0–3).
- **Four views, selected by `activeCamera(aspect)`.** Each frame this
  configures the active camera's projection/pose for the current aspect
  ratio and returns it as a single `const engine::Camera&`, which both
  `renderer.setCamera(cam)` and `MeshRenderer(queue, registry, cam)` consume
  identically — controller-owned and raw cameras work the same way:
  - **Orbit (0):** `m_orbit` auto-rotates — `setOrbit(40 + m_time * 18, 28,
    16)` each frame, target `(0, 0.5, 0)`.
  - **Fly / first-person (1):** `m_fly`'s pose (`{6, 4, 10}`, yaw −120°,
    pitch −18°) is set once in `onAttach`; `onUpdate` then drives it via
    `m_fly.update(dt)` (WASD + right-drag, from the existing controller).
  - **Top-down orthographic (2):** `m_top.lookAt({0, 14, 0.001f}, {0, 0,
    0})`. The `0.001f` z-offset on the eye is deliberate — a perfectly
    straight-down eye is parallel to the camera's hard-coded up vector,
    which degenerates `lookAt` into a NaN view matrix; the tiny offset
    keeps the view well-defined while still reading as top-down.
  - **Front fixed (3):** `m_front.setPerspective(55, aspect, 0.1, 100)` and
    a static `lookAt({0, 3.5, 14}, {0, 1, 0})` — no per-frame update.
- **View switching.** `Key::Space` cycles `m_view = (m_view + 1) % 4` in
  `onUpdate`; the engine's `Key` enum has no Tab or number keys, so Space is
  the switch key rather than the Tab/1–4 scheme sketched in the design
  spec. `BOTARENA_VIEW=0..3` (read in `onAttach`) sets the initial view for
  headless screenshots; the value is clamped to `[0, 3]` and defaults to 0
  when unset or out of range.
- **Scene**, submitted via `MeshRenderer`: a ground slab (a scaled unit
  cube), a ring of 8 cubes at radius 5 with four cycling materials, and a
  taller center pillar cube — all lit by four colored point lights so the
  materials read under every projection.
- **HUD**, an SDF font (`FontBackend::SDF`) laid out clear of the top-left
  debug overlay, exercising fill + outline + glow + shadow together over
  each projection:
  - Title ("Camera Views") with an outline.
  - The active view's name, large, with a glow.
  - The four view names stacked, the active one marked (`> `) and
    brightened, the rest dimmed.
  - An eye-position readout, `glm::inverse(cam.view())[3]` — generic across
    camera types, so it needs no per-camera `position()` accessor.
  - A controls hint ("Space: next view · WASD + right-drag: fly") with a
    drop-shadow.

## Scope for this slice

- **No engine code.** `OrbitCameraController`, `FlyCameraController`,
  `PerspectiveCamera`, `OrthographicCamera`, `MeshRenderer`,
  `Renderer::setCamera`, and the `drawText`/`TextStyle` API are used exactly
  as they exist post-v0.30.
- **Text stays screen-space.** This demo confirms the 2D HUD composites
  correctly over any 3D projection; it does not add world-space or
  billboard text — that remains a reserved future slice (see v0.30's `Next
  Milestones`).

## Testing

`BOTARENA_VIEW=<0..3> BOTARENA_SCREENSHOT=<path> ./build/camera_demo_game`
renders one frame in the given view and exits, capturing all four
headlessly. Visual inspection confirms the scene renders correctly under
each projection (orbit and fly in perspective, top-down orthographic
without the NaN degenerate case, fixed front) and that the HUD's
outline/glow/list/readout/shadow all stay legible over every one. No unit
tests are added, matching the other example games; the full suite is
unaffected since no engine code changed.

## Next Milestones

- World-space/billboard 3D labels anchored to scene objects, once the
  world-space text slice lands.
- Smooth animated transitions between camera views (currently a hard cut).
