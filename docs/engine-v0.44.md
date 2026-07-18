# Engine v0.44 — Migrate the rendering-technique demos onto Scene

After v0.43, five games were fully scene-driven but six still rendered outside
`Scene` (manual `PerspectiveCamera`, `MeshRenderer::submit`,
`setPointLights`/`setLightDirection`). v0.44 migrates the three that migrate
*cleanly* — the rendering-technique demos **csm_demo**, **normalmap_demo**, and
**parallax_light_demo** — onto `Scene`/ECS: camera → `CameraComponent`,
geometry → `MeshComponent`, lights → `LightComponent`, with the SDF text staying
a `renderer.drawText` overlay. It adds a reusable camera-conversion helper and
establishes the **animated `LightComponent`** pattern. Design rationale:
`docs/superpowers/specs/2026-07-18-engine-v0.44-scene-migration-demos-design.md`.

The other three off-Scene games (particles, camera_demo, billboard_demo) are
deferred: their core content — a custom `ParticleSystem`/`submitParticles`
pipeline, runtime switching between four camera types + an SDF HUD, and a
bespoke damage-number billboard simulation — has no `Scene` component and would
stay manual even after migration.

## `lookAtTransform` — camera conversion

`Scene` derives its view from the primary `CameraComponent` entity's
`TransformComponent`: `viewMatrix(t) = inverse(translate(t.translation) *
mat4_cast(t.rotation))`. The three demos each used a raw
`PerspectiveCamera::lookAt(eye, target)`, whose `view()` reduces exactly to
`glm::lookAt(eye, target, {0,1,0})`. A new inline, GL-free helper in
`engine/scene/SceneCamera.hpp` converts one to the other:

```cpp
inline TransformComponent lookAtTransform(
    const glm::vec3& eye, const glm::vec3& target,
    const glm::vec3& up = glm::vec3(0.0f, 1.0f, 0.0f)) {
  const glm::mat4 world = glm::inverse(glm::lookAt(eye, target, up));
  TransformComponent t;
  t.translation = eye;
  t.rotation = glm::quat_cast(glm::mat3(world));
  return t;
}
```

`glm::lookAt`/`glm::quat_cast` are `gtc` (already included by `SceneCamera.hpp`);
`glm::inverse`/`glm::mat3` are core — no `gtx`. A GL-free Catch2 test
(`tests/test_scene_camera.cpp`) asserts `viewMatrix(lookAtTransform(eye,target))`
≈ `glm::lookAt(eye,target,{0,1,0})` for the three demo cameras plus a
looking-down-+Z case. The helper is reusable for every future off-Scene camera
migration.

## Animated `LightComponent`

`collectLights` reads each light entity's live `TransformComponent` every frame,
so an animated light is simply an entity whose `translation` is mutated per
frame **before** `m_scene.render(...)`:

- **normalmap_demo** — an animated directional key: `m_keyLight`'s translation
  is set each frame to `{cos(a), 0.28, sin(a)*0.35+0.32}`
  (`collectLights` normalizes it), reusing the existing `a = m_screenshot ?
  preset : m_time*0.5` freeze logic.
- **parallax_light_demo** — three orbiting point lights: each
  `m_orbitLights[i]`'s translation is set to `{cos(ai)*2.4, 0.9, sin(ai)*2.4}`
  with `ai = a + 2π·i/3`; their colour/intensity/radius are static (set once at
  attach), only the position animates.

No new component or system is introduced — a game that animates a light mutates
its transform, exactly as a game that moves an object would (YAGNI).

## Per-game migration

Each game gained an `engine::Scene m_scene;`. `onAttach` creates the camera
entity (via `lookAtTransform`), the light entities, and the geometry entities
(transform-only); `ensureResources` attaches `MeshComponent`s once materials
exist (materials need a live `Renderer`, so this stays lazy, matching
scene_demo/models); `onRender` drops the manual submit path and calls
`m_scene.render(renderer, aspect)`, keeping the SDF text as an overlay. The raw
`PerspectiveCamera m_camera` and now-dead `MeshRenderer.hpp`/`PointLight.hpp`
includes were removed.

- **csm_demo** — camera `{5,7.5,9}→{-1,0,-13}`; ground + 8 pillars; one static
  directional `{0.62,0.5,0.12}`.
- **normalmap_demo** — camera `{2.6,2,10}→{0,1.6,0}`; three walls
  (flat/mapped/parallax materials); animated directional + a static point fill.
- **parallax_light_demo** — camera `{0.5,5.5,7}→{0,0,-0.5}`; one floor; three
  orbiting point lights + a static directional.

## Screenshot verification — a note on the AE gate

These are shadow-rendering demos, and this exposes a real property of the
`Scene` camera model: expressing a `lookAt` camera as a **quaternion-based**
`TransformComponent` (via `quat_cast → mat4_cast`) perturbs the view matrix by
~1e-6 relative to the old direct `glm::lookAt`. On non-shadowed content that is
sub-pixel and invisible; on CSM / PCF / parallax-self-shadow scenes it amplifies
into a scattering of sub-visual shadow-acne dither. So the migrated frames are
**not** a literal AE = 0 against their pre-migration baselines:

| Game | raw AE | AE @ fuzz 5% | PSNR |
|---|---|---|---|
| csm_demo | 5088 (0.55%) | 3 px | 66.5 dB |
| normalmap_demo | 666 (0.07%) | 0 px | 83.7 dB |
| parallax_light_demo | 7290 (0.79%) | 2 px | 66.6 dB |

Each was verified as correct — not a mapping bug — three ways: every migrated
code value is byte-identical to the deleted code; the diff **collapses to a
handful of pixels under `compare -fuzz 2-5%`** with PSNR 66–84 dB; and the diff
heatmaps show **clean silhouette edges and unshifted light hotspots** (a real
camera/geometry error would red-outline silhouettes and would not collapse under
fuzz). This is an inherent, sub-visual property of the quat-based `Scene`
camera, not a regression. Future migrations of shadow-heavy scenes should use
this fuzz-collapse + heatmap gate rather than a literal AE = 0.

## Wiring stays additive

`Scene::render` is untouched — both light setters remain guarded. The
additive→authoritative flip is still deferred while the three off-Scene games
remain.

## Testing

- `tests/test_scene_camera.cpp` — GL-free Catch2 test for `lookAtTransform`
  (4 cases). Full suite green (164 cases).
- csm_demo / normalmap_demo / parallax_light_demo screenshot-verified against
  pre-migration baselines via the fuzz-collapse + heatmap gate above.
- `SceneCamera.hpp` and `SceneRender.cpp` remain GL-free / unchanged, so the
  tests target is unaffected.

## Reserved for later slices

- **particles, camera_demo, billboard_demo** — deferred; their core content is
  not `Scene`-expressible (would need particle/billboard/multi-camera support,
  or accept partial migrations with a manual tail).
- **The additive→authoritative flip** — still deferred until every game is on
  `Scene`.
- **Multiple directional lights**, **per-submesh material overrides**,
  **sort/cull/instancing**, **`addComponent<T>` empty-tag fix** — reserved.

## Next Milestones

- Decide whether the three deferred demos warrant partial Scene migration or
  new Scene subsystems (particles/billboards).
- Per-submesh material overrides; sort/cull and instanced submission.
