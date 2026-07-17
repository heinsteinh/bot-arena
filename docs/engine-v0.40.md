# Engine v0.40 — MeshComponent + Scene::render

v0.40 closes the loop the v0.38 scene layer left open: entities with a
transform and a mesh now draw themselves. A new `MeshComponent` plus
`Scene::render(Renderer&, aspect)` submit every `<TransformComponent,
MeshComponent>` entity through the scene's primary camera, and `arena` +
`shooter` — the two gameplay-heavy games still running a bare
`entt::registry` with hand-rolled camera and submission code — migrate onto
`Scene`, `SceneObject`, `TransformComponent`, and `OrbitControllerComponent`.
Design rationale:
`docs/superpowers/specs/2026-07-17-engine-v0.40-mesh-component-design.md`.

## `MeshComponent` and `Scene::render`

`engine::MeshComponent` (`engine/scene/MeshComponent.hpp`) is a plain data
pair:

```cpp
struct MeshComponent {
  MeshHandle mesh = 0;
  MaterialHandle material = 0;
};
```

It lives in its own header — depending on `RenderCommand.hpp` for
`MeshHandle`/`MaterialHandle` (both `uint16_t`) — so `Components.hpp` stays
free of renderer dependencies.

`Scene::render(Renderer& renderer, float aspect)` (`engine/scene/Scene.hpp`,
implemented in `engine/scene/SceneRender.cpp`) draws the scene in three
steps: it computes `cameraUniforms(aspect)` from the primary `CameraComponent`
(as `cameraUniforms` already did before this slice) and pushes it via
`renderer.setCamera(cu)`; builds a `MatrixCamera` from the resulting
view/projection; and then iterates `registry.view<TransformComponent,
MeshComponent>()`, submitting each entity to an internal `MeshRenderer` as
`meshes.submit(m.mesh, m.material, t.localTransform())`. A scene with no
primary camera falls back to `cameraUniforms`'s identity view/projection,
same as before. Lighting stays outside `Scene::render` entirely — games
still call `renderer.setLightDirection`/`setPointLights` themselves before
`scene.render(...)`; a `LightComponent` is reserved but not implemented.

**`SceneRender.cpp` is engine-lib-only.** `Scene::render` needs a live GL
`Renderer`/`MeshRenderer`, so `SceneRender.cpp` is listed only in
`bot_arena_engine`'s sources in `CMakeLists.txt` — not in `bot_arena_tests`.
`Scene.cpp` (still in the tests target) stays renderer-free, and `Scene.hpp`
only forward-declares `class Renderer;`, never including a renderer header.
This keeps the GL-free `bot_arena_tests` target buildable and runnable
headless, exactly as the v0.38/v0.39 split intended — `Scene::render` is
screenshot-verified rather than unit-tested, since it has no meaningful
behavior without a real GL context.

## Scene-demo migration

`scene_demo` and `controller_demo` (`games/scene_demo/SceneDemoGame.cpp`,
`games/controller_demo/ControllerDemoGame.cpp`) both drop their manual
per-frame `MeshRenderer`/submit loop in `onRender` for one line,
`m_scene.render(renderer, aspect)`. Ground and cube `SceneObject`s pick up an
`engine::MeshComponent` once resources are ready (`ensureResources`, called
lazily on first `onRender` since materials need a live `Renderer`). Both
demos still set the light direction/point lights themselves before calling
`render`. This was a deliberately low-risk proof before touching the
gameplay-heavy games: screenshots are byte-identical to the v0.39 baselines.

## arena + shooter migration

`arena` and `shooter` both moved off an ad-hoc `entt::registry` with manual
`MeshRenderer` submission and a standalone `OrbitCameraController` onto
`engine::Scene`. The shape of the migration is the same in both games:

- **One transform, not two.** Each game's own `Transform` struct
  (`games/arena/Components.hpp`, `games/shooter/Components.hpp`) is deleted.
  Every entity carries the engine's `TransformComponent` instead, and
  gameplay code reads/writes it directly — no sync between a game transform
  and a scene transform. The field renames are mechanical: `.position` →
  `.translation`, and a scalar uniform `scale` becomes `.scale.x` (arena's
  wall/bot/player scaling, shooter's ship/bullet scaling, and the
  agent-collision/wall-bounce radius checks in `ArenaGame::stepSim` all read
  `tr.scale.x` as the old scalar radius).
- **Gameplay components stay put.** `Velocity`, `Health`, `Player`, `Bot`
  (arena) and `Enemy`, `Bullet` (shooter) remain defined in each game's
  `Components.hpp` and are emplaced directly on the scene's
  `entt::registry` (`m_scene.registry()`), alongside the engine's
  `TransformComponent`/`MeshComponent`. `Scene` doesn't know or care about
  them.
- **Static geometry and entities become `SceneObject`s.** Arena's four walls
  and ground, the player, and all bots are created via `m_scene.createObject(...)`;
  once a live `Renderer` exists (first `onRender`, since materials/mesh
  handles need one), each gets an `engine::MeshComponent{cube, material}`.
  Shooter's ground follows the same pattern.
- **Camera via `OrbitControllerComponent`.** Both games replace their
  standalone `OrbitCameraController` with a camera `SceneObject` carrying a
  `CameraComponent` (fov/near/far matched to the old controller's 60°/0.1/100
  so framing is unchanged) and an `OrbitControllerComponent`, driven every
  frame by `m_scene.update(dt)`.
- **Shooter ship facing.** The old controller's discrete yaw is now
  `t.rotation = glm::angleAxis(engine::headingToYaw(...), {0, 1, 0})`, set
  wherever the player/enemies/bullets pick a heading (input direction,
  auto-aim at the nearest enemy, seek-the-player, or bullet travel
  direction).

Behavior is preserved: `test_steering`, `test_wall_bounce`,
`test_agent_collision`, and `test_ship_controls` are all unchanged and green
(the underlying gameplay math was untouched — only the storage moved from a
game-local `Transform` to `TransformComponent`), arena's screenshot
composition matches its baseline, and shooter's pixel-diff against its
baseline is at most 1/255.

### Orbit yaw convention offset

`OrbitControllerComponent`'s yaw (v0.39) is measured from `+Z`, the FPS
convention `CameraMath.hpp` established. The old standalone
`OrbitCameraController` measured yaw from `+X` instead — a 90° offset.
Migrating an old `setOrbit(yaw, pitch, distance)` call means
`newYaw = oldYaw + 90` on the `OrbitControllerComponent`, not a direct copy.
Arena's old orbit used `yaw = 45`, which happens to land in the same place
either way (`sin(45°) == cos(45°)`, so swapping which axis yaw is measured
from is a no-op at exactly 45°) — the coincidence masked the offset there.
Shooter's old orbit used `yaw = 0`; its `OrbitControllerComponent` is seeded
with `yaw = 90` (`games/shooter/ShooterGame.cpp`) to land the camera at the
same position under the new convention.

### Known limitation: multi-submesh models don't fit `MeshComponent`

`MeshComponent` holds exactly one `{mesh, material}` pair, but shooter's ship
assets are multi-submesh `Model`s loaded via Assimp — the player's
`Viper.obj` has 3 submeshes, the elite enemy's `eliteship.obj` has 7 — each
with its own material. `MeshComponent` as designed in this slice cannot
represent that.

The shooter migration works around it with **render-only proxy
`SceneObject`s**: `ShooterGame::attachVisual` creates one child `SceneObject`
per submesh (each carrying just a `MeshComponent` for that submesh's mesh +
material) and records them in `m_visualParts`, a
`std::unordered_map<entt::entity, std::vector<entt::entity>>` from the owning
gameplay entity to its proxy entities. `syncVisual` re-derives one shared
transform from the owner's `TransformComponent` (folding in the model's
bounds-fit scale/offset and an `extraYaw` for the ship-model's baked-in
facing) and copies it onto every proxy each frame; the owner entity itself
never gets a `MeshComponent`. `attachMissingVisuals`/`syncAllVisuals` run
once per `onRender` to backfill proxies for anything spawned since the last
frame and resync all of them before `m_scene.render`. Destroying an owner
(`ShooterGame::destroyActor`) also destroys its proxy entities and erases the
`m_visualParts` entry, so bullets and dead enemies don't leak render-only
entities.

This is a real gap, not an incidental detail — it's the reason a
`ModelComponent` is the top reserved follow-up below.

### Known gap: `SceneObject::addComponent<T>` can't add empty tag components

`SceneObject::addComponent<T>` (`engine/scene/SceneObject.hpp`) returns
`T&`, forwarding to `entt::registry::emplace<T>(...)`. For empty tag types
(arena's `Player`/`Bot`, shooter's `Player`) `entt::registry::emplace`
returns `void`, not `T&`, so it can't go through that signature. Both games
work around it by emplacing tags directly on the registry —
`m_scene.registry().emplace<Player>(static_cast<entt::entity>(player))` —
bypassing `SceneObject::addComponent` entirely for tag types. Noted here as
a known gap in `SceneObject`, not fixed in this slice.

## Reserved for later slices

- **`LightComponent`** — lighting stays game-set (`setLightDirection`/
  `setPointLights` before `scene.render`); no scene-driven light entities
  yet.
- **`ModelComponent`** — a component holding a full multi-submesh `Model`
  that `Scene::render` would iterate directly, replacing the render-only
  proxy `SceneObject`/`m_visualParts` workaround shooter needed for its ship
  models.
- **Sort/cull + instancing in `Scene::render`** — the current loop submits
  every `<TransformComponent, MeshComponent>` entity unconditionally, in
  registry order.
- **The `SceneObject::addComponent<T>` empty-tag gap** — fix the signature
  (or provide an alternate call) so tag components don't require bypassing
  `SceneObject` for the registry directly.
- **Retiring standalone controllers** — `FlyCameraController`/
  `OrbitCameraController`/`CameraController` are still used by games that
  haven't migrated to scene controllers; arena and shooter no longer need
  them, but the classes themselves stay until every game moves off them.
- **Migrating the remaining games** — other games still run their own
  `entt::registry` and manual submission outside the `Scene` model.

## Next Milestones

- `LightComponent` and scene-driven lighting.
- `ModelComponent` for multi-submesh models, removing the shooter
  render-proxy workaround.
- Sort/cull and instanced submission in `Scene::render`.
