# Engine v0.42 — LightComponent (scene-driven lighting)

v0.41 finished making geometry scene-driven: `MeshComponent` and
`ModelComponent` entities are collected and submitted by `Scene::render`.
Lighting was not — every game still hand-set its lights each frame in
`onRender` via `renderer.setLightDirection(...)` / `renderer.setPointLights(...)`,
the same per-game boilerplate that the geometry components removed. v0.42
closes it: a data-only `LightComponent`, a pure collection pass, additive
`Scene::render` wiring, and two game migrations that prove both light paths.
Design rationale: `docs/superpowers/specs/2026-07-18-engine-v0.42-light-component-design.md`.

## `LightComponent`

`engine::LightComponent` (`engine/scene/LightComponent.hpp`) is a data-only
tagged struct, kept in its own header like `MeshComponent`/`ModelComponent` so
`Components.hpp` stays free of renderer/asset dependencies:

```cpp
enum class LightType { Directional, Point };

struct LightComponent {
  LightType type      = LightType::Point;
  glm::vec3 color     = glm::vec3(1.0f);
  float     intensity = 1.0f;
  float     radius    = 10.0f;  // point lights only; ignored for directional
};
```

One tagged struct rather than two components: a game thinks in terms of "a
light," it mirrors the single-struct shape of the geometry components, and the
collection pass walks a single view. A directional light ignores `radius`.

## `collectLights` — the pure collection pass

`engine::collectLights` (`engine/scene/LightCollection.hpp`) is an inline,
GL-free free function — header-only like `modelRenderTransform`, so it needs no
new `.cpp` in the GL-free `bot_arena_tests` source list:

```cpp
struct CollectedLights {
  std::vector<PointLight> points;
  bool      hasDirectional = false;
  glm::vec3 directionalDir  = glm::vec3(0.0f, 1.0f, 0.0f);
};

CollectedLights collectLights(const entt::registry& reg);
```

It walks `<TransformComponent, LightComponent>`:

- **Point light** — position is `TransformComponent.translation`, radius is
  `LightComponent.radius`, and the packed `PointLight` carries
  `positionRadius = vec4(translation, radius)`, `color = vec4(color, intensity)`.
  No cap or sort here — the renderer already does nearest-N by camera distance.
- **Directional light** — direction is `normalize(TransformComponent.translation)`,
  the same "toward-light" vector games passed to `setLightDirection` before,
  now expressed as the light entity's translation. **The first directional in
  view order wins**; later directionals are ignored, because the renderer
  honors exactly one directional light (it drives CSM shadows and the IBL
  environment).

The contract is unit-tested (`tests/test_light_collection.cpp`, Catch2): point
lane packing, directional normalize, first-directional-wins, the empty scene,
and a mixed scene.

## `Scene::render` wiring is additive, on purpose

`SceneRender.cpp` (engine-lib-only, the same GL-isolation boundary v0.40
established) collects lights right after `setCamera` and pushes them **only if
the scene declares them**:

```cpp
const CollectedLights cl = collectLights(m_registry);
if (!cl.points.empty()) renderer.setPointLights(cl.points);
if (cl.hasDirectional)  renderer.setLightDirection(cl.directionalDir);
```

Both setters are guarded. This is load-bearing: during this slice only
scene_demo and arena carry light entities, and the other ~8 light-setting games
still call `setPointLights`/`setLightDirection` in their own `onRender`
*before* `m_scene.render(...)`. An unconditional `setPointLights(cl.points)`
would wipe those games' manually-set lights every frame, since their
`cl.points` is empty. Guarding on `!cl.points.empty()` (and `hasDirectional`)
makes `Scene::render` a lighting no-op for any scene without light entities, so
un-migrated games are completely unaffected. A later slice that migrates *every*
light-setting game could switch to an authoritative rule (always set, clearing
what the scene omits); until then, additive is required.

## Migrations

**scene_demo** (directional path). Its two manual calls
(`setLightDirection(normalize({0.5,0.7,0.35}))` and `setPointLights({})`) are
replaced by one directional `SceneObject` created in `onAttach` — translation
`{0.5, 0.7, 0.35}`, `LightComponent{ LightType::Directional, ... }`.
`collectLights` normalizes the translation to the identical vector, so the
frame is pixel-identical (screenshot AE = 0).

**arena** (point path). Its per-frame `std::vector<engine::PointLight>` build
and `setPointLights` call are replaced by four point `SceneObject`s created
once in `onAttach`, each with translation `{±4, 2, ±4}` and
`LightComponent{ LightType::Point, palette[i], 3.0f, 8.0f }`. The four
collected `PointLight`s are value-identical to the old hand-built set, and the
renderer sums point-light contributions (order-independent for a fixed set), so
the lit frame is unchanged (screenshot AE = 0). The now-dead
`#include "engine/renderer/PointLight.hpp"` was dropped from `ArenaGame.cpp`.

## Testing

`collectLights` is a GL-free Catch2 unit test in `bot_arena_tests`. The render
path itself has no meaningful behavior without a live GL context, so both
migrations are screenshot-verified against pre-migration baselines (AE = 0).
`SceneRender.cpp` remains engine-lib-only, so the new `LightCollection.hpp`
include does not leak GL into the tests target.

## Reserved for later slices

- **Zero-translation directional guard** — `collectLights` calls
  `glm::normalize(translation)` on a directional light unconditionally; a
  directional entity left at the default `{0,0,0}` translation would yield NaN.
  No current caller hits it (a directional must set a translation), but a
  length guard + test is worth a follow-up for a data-driven light system.
- **Multiple directional lights** — the renderer honors one; `collectLights`
  keeps the first and ignores the rest. No priority/aggregation beyond view
  order.
- **Migrating the remaining light-setting games** — shooter, models, particles,
  camera_demo, billboard_demo, normalmap_demo, parallax_light_demo, csm_demo,
  controller_demo still set lights manually. Only scene_demo + arena migrate
  here (one screenshot-gated proof per light path).
- **Authoritative (clearing) wiring** — once every game is migrated,
  `Scene::render` could own lighting fully instead of being additive.
- **Sort/cull + instancing** and the **`addComponent<T>` empty-tag fix** remain
  reserved from prior slices.

## Next Milestones

- Migrate the remaining games onto `LightComponent`; switch the wiring from
  additive to authoritative.
- Zero-vector directional hardening.
- Per-submesh material overrides; sort/cull and instanced submission in
  `Scene::render`.
