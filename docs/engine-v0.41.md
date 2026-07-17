# Engine v0.41 — ModelComponent (multi-submesh scene rendering)

v0.40 gave `Scene::render` a `MeshComponent` path, but `MeshComponent` holds
exactly one `{mesh, material}` pair — it cannot represent a multi-submesh
`Model` (Viper = 3 submeshes, eliteship = 7). Shooter worked around that gap
with render-only proxy `SceneObject`s, one per submesh, resynced every frame.
v0.41 closes it properly: a `ModelComponent` draws a whole `Model` through a
second `Scene::render` loop, `ResourceRegistry` gains a model store to back
it, `models_game` migrates onto `Scene` as an isolated proof, and shooter's
proxy machinery — the v0.40 debt — is deleted outright. Design rationale:
`docs/superpowers/specs/2026-07-17-engine-v0.41-model-component-design.md`.

## `ModelHandle` and the `ResourceRegistry` model store

`RenderCommand.hpp` gains `using ModelHandle = uint16_t;` beside the existing
`MeshHandle`/`MaterialHandle`. `ResourceRegistry` gains the matching pair,
following the same pattern as `registerMesh`/`registerMaterial`:

```cpp
ModelHandle registerModel(const Model& model);   // stores a copy, returns index
const Model& model(ModelHandle handle) const;    // lookup
```

`registerModel` pushes onto a private `std::vector<Model>` and returns the
new index; `model` looks the handle back up. Handles are assumed valid, same
contract as `registerMesh`/`registerMaterial` — an out-of-range handle is a
programming error, not a checked failure.

## The `Model.hpp` extraction

`Submesh` and `Model` used to live inline in `ModelLoader.hpp`. `ResourceRegistry`
needs `Model` for its new store, but `ModelLoader` already needs
`ResourceRegistry` (to register the meshes/materials it loads) — including
`ModelLoader.hpp` from `ResourceRegistry.hpp` would create a cycle. The fix is
a new data-only header, `engine/assets/Model.hpp`:

```cpp
struct Submesh { MeshHandle mesh = 0; MaterialHandle material = 0; };
struct Model   { std::vector<Submesh> submeshes; AABB bounds{}; bool valid = false; };
```

`ModelLoader.hpp` now `#include`s this instead of defining `Submesh`/`Model`
inline, and keeps `loadModel(...)`. `Model.hpp` never includes
`ResourceRegistry.hpp`, so the cycle is broken at the data layer:
`ResourceRegistry` depends only on the plain-data `Model`, not on the loader
that produces one.

## `ModelComponent` and `modelRenderTransform`

`engine::ModelComponent` (`engine/scene/ModelComponent.hpp`) is a plain data
struct, kept in its own header for the same reason `MeshComponent` is —
`Components.hpp` stays free of renderer/asset dependencies:

```cpp
struct ModelComponent {
  ModelHandle    model = 0;
  bool           normalized = true;        // fit bounds into a unit cube
  MaterialHandle materialOverride = 0;     // 0 = baked submesh materials; else tint all
  glm::quat      rotationOffset{1.0f, 0.0f, 0.0f, 0.0f};  // render-only facing fix
};
```

The world matrix each submesh renders with is computed by a new pure, GL-free
helper, `modelRenderTransform` (`engine/scene/ModelTransform.hpp`):

```cpp
glm::mat4 modelRenderTransform(const TransformComponent& t,
                                const ModelComponent& mc,
                                const AABB& bounds) {
  const glm::quat rot = t.rotation * mc.rotationOffset;
  glm::mat4 m = glm::translate(glm::mat4(1.0f), t.translation) *
                glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), t.scale);
  if (mc.normalized) m = m * fitToUnitTransform(bounds);
  return m;
}
```

Its unit test (`tests/test_model_transform.cpp`) asserts this is algebraically
identical to the v0.40 shooter proxy's collapsed formula —
`translate(pos + rot*(-uni*center)) * mat4_cast(rot) * scale(uni)` — so the
transform correctness that used to be re-derived by hand for the proxy is now
pinned by a test instead.

## `Scene::render` model path

`SceneRender.cpp` (engine-lib-only, same GL-isolation boundary v0.40
established) adds a second loop, after the existing
`<TransformComponent, MeshComponent>` one, over
`<TransformComponent, ModelComponent>`: it looks up the `Model` by handle,
calls `modelRenderTransform`, and submits every submesh with
`materialOverride != 0 ? materialOverride : submesh.material`. The two loops
are independent — an entity can carry a `MeshComponent`, a `ModelComponent`,
or neither, and `Scene::render` draws whichever it has.

## models_game migration

`games/models/ModelsGame.cpp` moves onto `Scene` + `ModelComponent` as an
isolated, low-risk proof before touching a gameplay-heavy game: the standalone
`OrbitCameraController` and the manual `MeshRenderer` submit loop are replaced
with a camera `SceneObject` (`CameraComponent` + `OrbitControllerComponent`)
and a model `SceneObject` carrying `ModelComponent`; `onRender` calls
`m_scene.render(renderer, aspect)` instead of hand-looping submeshes. The
ImGui model-cycling viewer keeps working by reassigning the entity's
`ModelComponent.model` in place rather than recreating the entity. Screenshot
diff against the pre-migration baseline is AA/rounding noise only (232/921600
pixels, 0.025%), and the camera lands at the identical position (see the orbit
yaw note below).

## shooter proxy retirement

Shooter's workaround from v0.40 — `m_visualParts` (a
`std::unordered_map<entt::entity, std::vector<entt::entity>>` from owner to
proxy entities), `attachVisual`, `syncVisual`, `attachMissingVisuals`,
`syncAllVisuals`, and the proxy-cleanup half of `destroyActor` — is deleted
outright (roughly 72 net fewer lines). In its place: one idempotent
`attachMissingModels()` that walks `Player`/`Enemy`/`Bullet` views and
emplaces a `ModelComponent` on any actor that doesn't have one yet, called
once per `onRender`. `destroyActor` collapses to:

```cpp
void ShooterGame::destroyActor(entt::entity e) {
  entt::registry& reg = m_scene.registry();
  if (reg.valid(e)) reg.destroy(e);  // ModelComponent is destroyed with it
}
```

There is no per-frame proxy sync anymore — `Scene::render` reads the actor's
live `TransformComponent` directly, so there's nothing left to keep in sync.
Screenshot diff against the v0.40 shooter baseline is an exact pixel match
(AE = 0); the gameplay tests (`test_steering`, `test_wall_bounce`,
`test_agent_collision`, `test_ship_controls`) are unchanged and green.

### `rotationOffset` is render-only, on purpose

The ship's 180° facing flip (`kShipYaw`) lives only in
`ModelComponent.rotationOffset` — never in `TransformComponent.rotation`.
Gameplay reads the transform's rotation back to aim (`fwd = playerRot * +Z`,
and bullets inherit the firing entity's rotation), so folding the flip into
the transform itself would fire ships backward. This corrected the design
spec's original assumption ("fold `kShipYaw` into the entity rotation"),
caught during planning rather than after a screenshot regression — the spec
was amended before implementation started.

### `materialOverride == 0` is a pragmatic sentinel

`0` means "use each submesh's baked material"; any other handle tints every
submesh with that material (shooter's bullet team color). This collides with
the fact that `0` is also a valid, real material handle, but overriding a
model's submeshes to a game's first-registered material is a nonsensical
thing to want in practice, so treating `0` as "no override" is safe here. If
a genuine need for overriding *to* handle `0` ever comes up, a later slice
would need an explicit `bool hasOverride` instead.

### Lazy attach, no per-frame sync

Shooter's `onAttach` warm-up spawns actors before models are registered in
`onRender` (registration needs a live `Renderer`), so `ModelComponent` can't
be attached at spawn time. `attachMissingModels()` runs once per frame and is
idempotent — it only fills in entities missing the component — rather than
running a per-frame transform sync the way `syncAllVisuals` used to. That
sync loop is gone entirely in this slice: `Scene::render` already reads the
live `TransformComponent`, so there is nothing to resync.

### Orbit yaw convention (models_game)

Mapping an old `OrbitCameraController::setOrbit(yaw, pitch, distance)` call
onto `OrbitControllerComponent` is `Ynew = 90 - Yold`, not `Yold + 90` — the
two only coincide at `Yold = 0`. `OrbitControllerComponent`'s
`orbitPosition` yields `pos = center + d*(cosP*sinY, sinP, cosP*cosY)`; the
old controller yielded `pos = center + d*(cosP*cosYold, sinP, cosP*sinYold)`.
Solving `sinY = cosYold` and `cosY = sinYold` gives the `90 - Yold` relation.
models_game's old `setOrbit(35, 20, 3.5)` maps to `oc.yaw = 55`.

## Testing

`registerModel`/`model` (`tests/test_resource_registry.cpp`) and
`modelRenderTransform` (`tests/test_model_transform.cpp`) are GL-free unit
tests in `bot_arena_tests`. The render path itself has no meaningful behavior
without a live GL context, so it stays screenshot-verified, same as v0.40's
`MeshComponent` path: models_game against its pre-migration baseline, shooter
against the v0.40 baseline. `SceneRender.cpp` remains engine-lib-only, so the
GL-free tests target is unaffected by the new model loop.

## Reserved for later slices

- **`LightComponent`** — lighting still stays game-set
  (`setLightDirection`/`setPointLights` before `scene.render`); untouched by
  this slice.
- **Per-submesh material overrides** — `materialOverride` is all-or-nothing
  per model; a mesh→material map would allow tinting individual submeshes.
- **Sort/cull + instancing in `Scene::render`** — both the mesh loop and the
  new model loop submit unconditionally, in registry order.
- **The `SceneObject::addComponent<T>` empty-tag fix** — noted in v0.40,
  still not fixed.
- **Migrating the remaining games** — particles, camera_demo, and
  billboard_demo still run outside the `Scene` model.

## Next Milestones

- `LightComponent` and scene-driven lighting.
- Per-submesh material overrides.
- Sort/cull and instanced submission in `Scene::render`.
