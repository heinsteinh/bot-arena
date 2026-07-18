# Engine v0.43 — Finish the LightComponent migration (Scene-based games)

v0.42 shipped `LightComponent` and the additive `Scene::render` collection
pass, but migrated only scene_demo (directional) and arena (point) as proofs.
v0.43 finishes the job for every game that is *already on `Scene`*: shooter,
models, and controller_demo move their hand-set `onRender` lights onto
`LightComponent` entities, and `collectLights` is hardened against the
zero-vector directional NaN noted as v0.42's first Minor. Design rationale:
`docs/superpowers/specs/2026-07-18-engine-v0.43-light-migration-design.md`.

## Why only three games

`LightComponent` is collected by `Scene::render`; a game that never calls
`m_scene.render(...)` cannot use it. After this slice, all five Scene-based
games are scene-driven for lighting: scene_demo, arena, shooter, models,
controller_demo. The six remaining light-setting games — particles,
camera_demo, billboard_demo, normalmap_demo, parallax_light_demo, csm_demo —
are **not on `Scene`** (they render manually), so their lighting cannot migrate
until they are first migrated onto `Scene` (a separate, larger effort). They
keep their manual `onRender` light calls, which the additive wiring leaves
untouched.

## Zero-vector directional guard

`collectLights` computed `glm::normalize(t.translation)` unconditionally for a
directional light. A directional entity left at the default `{0,0,0}`
translation (a misconfiguration) would yield `normalize({0,0,0})` = NaN/Inf and
silently poison `setLightDirection` — and thus CSM shadows and the IBL
environment. The directional branch is now guarded:

```cpp
} else if (!out.hasDirectional &&
           glm::dot(t.translation, t.translation) > kMinDirLen2) {
  out.hasDirectional = true;
  out.directionalDir = glm::normalize(t.translation);
}
```

- `glm::dot(v, v)` is squared length using **core GLM only** — `glm::length2`
  lives in `glm/gtx/norm.hpp`, and this codebase bans `gtx` (the
  `GLM_ENABLE_EXPERIMENTAL` trap).
- `kMinDirLen2 = 1e-12f` filters only a true-zero / denormal translation; the
  smallest real direction in use (`{0.5,0.7,0.35}`, squared length ≈ 0.86) is
  twelve orders of magnitude clear of it.
- Because the guard is ANDed into the existing `!out.hasDirectional` gate, a
  degenerate directional is **ignored**: it neither sets `hasDirectional` nor
  consumes the "first directional wins" slot, so a later valid directional in
  the same scene still wins, and a scene whose only directional is degenerate
  reports `hasDirectional=false` (the additive contract then leaves the
  renderer's directional at its default).

Two Catch2 cases cover it: a lone zero-translation directional (`hasDirectional`
stays false), and a zero directional followed by a valid one (the valid one
wins). The five pre-existing `collectLights` cases are unchanged.

## The three migrations

Same mechanical pattern as scene_demo/arena, and — because a `LightComponent`
needs no `Renderer` — the light entity is created directly in `onAttach` with
no lazy-attach path (even shooter's pre-resource warm-up is fine). The manual
`onRender` light calls are deleted; each is screenshot-verified at **AE = 0**
because `collectLights` reproduces the identical `PointLight` / directional
vector.

| Game | Light entity | Reproduces |
|---|---|---|
| **shooter** | Point, translation `{0,8,4}`, color `{1,0.97,0.9}`, intensity `3.0`, radius `40` | `positionRadius=(0,8,4,40)`, `color=(1,0.97,0.9,3.0)` |
| **models** | Point, translation `{2,3,2}`, color `{1,0.97,0.9}`, intensity `2.5`, radius `15` | `positionRadius=(2,3,2,15)`, `color=(1,0.97,0.9,2.5)` |
| **controller_demo** | Directional, translation `{0.5,0.7,0.35}` | `setLightDirection(normalize(0.5,0.7,0.35))` |

The now-dead `#include "engine/renderer/PointLight.hpp"` was dropped from
shooter and models (controller_demo never had it — it used `setPointLights({})`
with no `PointLight` type). shooter's gameplay tests (steering, ship controls,
wall bounce, combat) confirm the lighting change did not disturb simulation.

## Wiring stays additive

`Scene::render` is untouched — both setters remain guarded
(`if (!cl.points.empty())`, `if (cl.hasDirectional)`). The
additive→authoritative flip's precondition (every game on `Scene`) is still
unmet while the six off-Scene demos exist, so it is deferred. Once those games
are on `Scene`, the flip becomes a one-line change: delete the two guards.

## Testing

- `tests/test_light_collection.cpp` — 7 Catch2 cases total (5 prior + 2 guard),
  GL-free. Full suite green (160 cases).
- shooter, models, controller_demo each screenshot-verified AE = 0 vs a
  pre-migration baseline.
- `SceneRender.cpp` unchanged, so `bot_arena_tests` remains GL-free.

## Reserved for later slices

- **The six off-Scene games** — blocked on a prior `Scene` migration; they keep
  their manual light calls.
- **The additive→authoritative flip** — deferred until every game is on
  `Scene`.
- **Multiple directional lights**, **per-submesh material overrides**,
  **sort/cull/instancing**, and the **`addComponent<T>` empty-tag fix** remain
  reserved.

## Next Milestones

- Migrate the six remaining games onto `Scene` (geometry + lighting), then flip
  `Scene::render` lighting from additive to authoritative.
- Per-submesh material overrides; sort/cull and instanced submission.
