# Engine v0.45 — Partial-migrate the last 3 games onto Scene

v0.44 left three games off `Scene` — particles, camera_demo, billboard_demo —
because each has *core content* `Scene` has no component for: a custom
`ParticleSystem`/`submitParticles` pipeline, runtime switching between four
camera types + an SDF HUD, and a bespoke damage-number billboard simulation.
v0.45 **partially** migrates all three: their camera, geometry, and lights move
onto `Scene` (`CameraComponent` + `MeshComponent` + `LightComponent`, drawn by
`m_scene.render`), while their bespoke content runs as a manual tail after the
render call. This removes the last per-game `MeshRenderer`/`setPointLights`
boilerplate. Design rationale:
`docs/superpowers/specs/2026-07-18-engine-v0.45-partial-scene-migration-design.md`.

**Milestone:** after this slice **every game is on `Scene` and none sets lights
manually**, which unblocks the additive→authoritative `Scene::render` flip
(deferred to its own future slice).

## `cameraTransformFromView` — the general camera-conversion helper

v0.44's `lookAtTransform` builds a `TransformComponent` from an `(eye, target)`
pair. Some cameras aren't a clean eye/target lookAt (a fly camera, an
orthographic top-down), so v0.45 adds the general form in
`engine/scene/SceneCamera.hpp` — build the transform from any rigid *view
matrix*:

```cpp
inline TransformComponent cameraTransformFromView(const glm::mat4& view) {
  const glm::mat4 world = glm::inverse(view);   // = translate * rotate
  TransformComponent t;
  t.translation = glm::vec3(world[3]);
  t.rotation = glm::quat_cast(glm::mat3(world));
  return t;
}
```

GL-free, `gtc`/core only, unit-tested (`viewMatrix(cameraTransformFromView(V))
≈ V` for a perspective and a top-down view).

## Camera: game-owned, driving the Scene camera

A partial migration keeps each game's bespoke camera logic; the camera just
drives the single primary `CameraComponent` entity instead of
`renderer.setCamera`.

- **billboard_demo, particles** used a *static* `OrbitCameraController`. Since
  the orbit never moves, `eye = target + offset·distance` (with `offset =
  {cos(pitch)cos(yaw), sin(pitch), cos(pitch)sin(yaw)}`) is computed once in
  `onAttach` and fed to `lookAtTransform` with a static `CameraComponent{fov=60}`
  — **retiring their `OrbitCameraController`** entirely.
- **camera_demo** keeps its four cameras + runtime switch. Each frame the active
  camera's `view()` syncs into the Scene camera via `cameraTransformFromView`,
  and the `CameraComponent` projection is set per view: perspective
  (`fov=60` orbit/fly, `fov=55` front) or **orthographic** top-down
  (`orthoSize=24`, `orthoNear=-100`, `orthoFar=100`, reproducing the old
  `setBounds(-12·aspect, 12·aspect, -12, 12, -100, 100)`).

## Per-game migration

Each game gained `engine::Scene m_scene;`. Geometry → `MeshComponent` entities
(materials attached lazily once a `Renderer` exists), lights → `LightComponent`
entities, `onRender` → `m_scene.render(renderer, aspect)`, and the bespoke
content follows as a manual tail. The manual submit loop and now-dead
`MeshRenderer.hpp`/`PointLight.hpp` includes were removed.

- **billboard_demo** — static camera; ground + 6 static enemy cubes; 1 point
  light. Manual tail: damage-number `cameraBillboard` text + the screen HUD.
- **particles** — static camera; ground + 6 bouncer cubes whose entity
  `TransformComponent.translation` **syncs from the physics each frame** (the
  same live-transform pattern as v0.44's animated lights); 1 point light. Manual
  tail: `submitParticles` + the ImGui editor.
- **camera_demo** — per-frame camera sync; ground + 8 rotated ring cubes
  (`TransformComponent.rotation = angleAxis(a, +Y)`) + a center pillar; 4 point
  lights. Manual tail: the SDF HUD (its `eye` readout still reads the active
  manual camera).

## Screenshot verification — and a debug-overlay footnote

As in v0.44, expressing a camera as a quat-based `TransformComponent` perturbs
the view by ~1e-6, so migrated frames are verified with the fuzz-collapse +
heatmap gate rather than a literal AE = 0. All four migrated frames pass:

| Frame | raw AE | AE @ fuzz 5% | PSNR |
|---|---|---|---|
| billboard_demo | 904 (0.10%) | 1 px | 73.0 dB |
| particles | 844 (0.09%) | 1 px | 72.5 dB |
| camera_demo (orbit, default) | 698 (0.08%) | 0 px | 82.5 dB |
| camera_demo (top-down ortho) | 1535 (0.17%) | 1093 px* | 43.4 dB* |

\* camera_demo's top-down view is a special case worth recording. Its raw diff
does *not* collapse under fuzz — but masking the top-left engine debug overlay
drops the rest of the frame (scene + HUD) to **AE = 1 / PSNR 113.8 dB**
(pixel-identical). The entire residual is the debug overlay's `Cam:` position
readout: the overlay prints the *reconstructed Scene* camera position, which —
against the deliberately tiny top-down `eye z = 0.001` — is ~1e-6 off the
original and flips a printed digit. The demo's own HUD reads the *unchanged
manual* camera and matches. This is a cosmetic engine-chrome float artifact of
the (v0.44-accepted) quat camera representation, not a migration defect: the
rendered scene and projection are pixel-exact. It's a nice reminder that the
debug overlay is a sensitive numeric oracle — it surfaced the sub-visual camera
difference as text.

## Wiring stays additive

`Scene::render` is untouched. Every game now calls it and none sets lights
manually, so the additive→authoritative flip is unblocked but deferred.

## Testing

- `tests/test_scene_camera.cpp` gains `cameraTransformFromView` cases (GL-free,
  Catch2). Full suite green: 166 cases / 50904 assertions.
- Each migrated frame verified with the fuzz-collapse + heatmap gate above.
- `SceneCamera.hpp` stays GL-free; `SceneRender.cpp` unchanged.

## Reserved for later slices

- **The additive→authoritative flip** — now unblocked (all games on Scene +
  light-driven); a one-line change to the two guards in `SceneRender.cpp`, its
  own slice.
- **Native `Scene` support for particles / billboards / multi-camera switching**
  — not built; those remain manual tails by design.
- **Retiring `FlyCameraController`/`OrthographicCamera`** — camera_demo still
  uses them (synced into the Scene camera).
- **Per-submesh material overrides, sort/cull/instancing, `addComponent<T>`
  empty-tag fix** — reserved.

## Next Milestones

- The additive→authoritative lighting flip.
- Per-submesh material overrides; sort/cull and instanced submission in
  `Scene::render`.
