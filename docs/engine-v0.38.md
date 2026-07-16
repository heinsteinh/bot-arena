# Engine v0.38 — Scene Foundation

v0.38 adds a minimal, data-oriented scene layer on top of the EnTT dependency the
engine already carried: a `Scene` that owns one `entt::registry`, a `SceneObject`
handle facade over it, and a small set of components (`IDComponent`, `TagComponent`,
`TransformComponent`, `CameraComponent`). The renderer gets one additive overload so
a scene's primary camera can drive a frame without the renderer ever learning what a
`Scene` is. A new `scene_demo` proves the whole path end to end. Design rationale:
`docs/superpowers/specs/2026-07-16-engine-v0.38-scene-foundation-design.md`.

## `Scene` and `SceneObject`

`Scene` (`engine/scene/Scene.hpp`) owns exactly one `entt::registry` and an
ID counter:

- `createObject(name)` creates an entity and auto-attaches `IDComponent` (a
  monotonically increasing `uint64_t`, starting at 1), `TagComponent` (defaults to
  `"SceneObject"` when `name` is empty), and a default-constructed
  `TransformComponent`.
- `destroyObject(SceneObject)` destroys the underlying entity.
- `primaryCamera()` scans the `CameraComponent` view and returns the first object
  whose `CameraComponent::primary` is `true` (a default-constructed `SceneObject` if
  none is found).
- `cameraUniforms(aspect)` finds that same primary camera and returns a
  `CameraUniforms` built from its `TransformComponent` and `CameraComponent` (falls
  back to identity matrices if there's no primary camera).
- `registry()` exposes the underlying `entt::registry&` for callers that need raw
  EnTT access.

`SceneObject` (`engine/scene/SceneObject.hpp`) is a small non-owning handle: just
`{entt::entity, Scene*}`, nothing else. It has no component data of its own — every
operation (`addComponent`, `getComponent`, `hasComponent`, `removeComponent`, all
templated) forwards to the owning `Scene`'s registry via a `friend class SceneObject`
grant on `Scene`. `explicit operator bool()` reports whether the handle still points
at a live entity in a live scene (`m_scene != nullptr && m_scene->m_registry.valid(...)`),
so a `SceneObject` correctly goes false after `destroyObject`. `operator entt::entity()`
lets it drop into raw EnTT calls. There is deliberately no second, parallel ownership
tree: `Scene` is the only owner, `SceneObject` is a view onto it.

## Data-oriented components

`engine/scene/Components.hpp` defines the four components as plain data, no
inheritance and no virtual dispatch:

- `IDComponent { uint64_t id }`, `TagComponent { std::string name }`.
- `TransformComponent { vec3 translation; vec3 rotation (euler radians, pitch/yaw/roll);
  vec3 scale; }`, with `localTransform()` composing `T * R * S` via
  `glm::translate(...) * glm::mat4_cast(glm::quat(rotation)) * glm::scale(...)`.
  `TransformComponent` is the **single source of truth for pose** — nothing else on
  an object stores a position or orientation.
- `CameraComponent { ProjectionType type; fov; perspNear/perspFar; orthoSize;
  orthoNear/orthoFar; aspect; primary; fixedAspectRatio }` — **projection-only**. It
  carries no translation or rotation of its own; a camera's view matrix always comes
  from that same object's `TransformComponent`.

`engine/scene/SceneCamera.hpp` supplies the two free functions that turn those
components into matrices: `viewMatrix(const TransformComponent&)` builds
`translate * rotate` (scale is intentionally excluded from a view matrix) and
inverts it, and `projectionMatrix(const CameraComponent&, float aspect)` dispatches
on `ProjectionType` to `glm::perspective` or `glm::ortho`, using the component's own
`aspect` instead of the passed-in one when `fixedAspectRatio` is set.

This data-oriented shape was a deliberate choice over a transitional
`shared_ptr<Camera>`-holding `CameraComponent`: routing straight to plain data means
there is only ever one place a camera's pose can live (`TransformComponent`), so the
"two sources of truth for camera pose" failure mode the design spec calls out cannot
arise even temporarily. `Scene::cameraUniforms` and `tests/test_scene.cpp` both take
advantage of this by reading translation straight off `TransformComponent` and
asserting it against the resulting `cameraPosition`.

## Renderer integration

`Renderer::setCamera` (`engine/renderer/Renderer.hpp`) gains a second, additive
overload:

```cpp
void setCamera(const Camera& camera) {
  m_camera = makeCameraUniforms(camera.view(), camera.projection());
}
void setCamera(const CameraUniforms& uniforms) { m_camera = uniforms; }
```

The new overload takes a `CameraUniforms` directly, so a `Scene`'s
`cameraUniforms(aspect)` output can be handed to the renderer with no adapter. The
original `setCamera(const Camera&)` is untouched, and the renderer has no idea a
`Scene` exists — it only ever sees `CameraUniforms`.

For call sites that still need a `const Camera&` (like `MeshRenderer`'s
constructor), `engine/renderer/MatrixCamera.hpp` adds `MatrixCamera`: a `Camera`
subclass that stores precomputed `view`/`projection` matrices and returns them
verbatim from `view()`/`projection()`. It exists purely as a bridge so scene-driven
matrices can flow into APIs that were written against `Camera`, without those APIs
changing. Every existing camera class (`PerspectiveCamera`, `OrthographicCamera`,
`Camera2D`) and every existing game (`arena`, `shooter`, and the other demos) is
unmodified — the only engine-wide change in this slice is the one new overload.

## Unit testing

The scene layer is pure CPU logic plus header-only EnTT, so it is fully covered by
CPU unit tests with no rendering context required. `tests/test_scene.cpp` adds 11
cases:

- `localTransform` matches a hand-built `T*R*S` matrix.
- `viewMatrix` matches `inverse(translate*rotate)`, and the recovered camera
  position equals the original translation.
- `projectionMatrix` matches `glm::perspective`/`glm::ortho` for both projection
  types, and `fixedAspectRatio` overrides the caller-supplied aspect.
- `createObject` auto-attaches `IDComponent`/`TagComponent`/`TransformComponent`;
  IDs are unique and increasing; the default tag is `"SceneObject"`.
- `addComponent`/`getComponent`/`hasComponent`/`removeComponent` round-trip on
  `CameraComponent`.
- `operator bool` is false for a default-constructed handle and for one that has
  been through `destroyObject`.
- `primaryCamera()` returns an invalid handle when no camera (or no *primary*
  camera) exists, and returns the right object once one is added with
  `primary = true`.
- `cameraUniforms(aspect)` composes `viewMatrix`/`projectionMatrix` into a
  `CameraUniforms` matching `makeCameraUniforms` built independently, and its
  `cameraPosition` matches the source `TransformComponent::translation`.

All matrix comparisons are against independently hand-built GLM matrices, not
against the production code path, so the tests catch regressions in either
`SceneCamera.hpp` or `Scene::cameraUniforms` independently. The suite grew from 130
to 141 cases; `Scene.cpp` was added to both the engine library and the tests target,
and `EnTT::EnTT` is now linked into `bot_arena_tests` (it wasn't previously).

## `scene_demo`

`scenedemo::SceneDemoGame` (`scene_demo_game` target,
`games/scene_demo/SceneDemoGame.cpp`) creates a `Scene` with a primary camera object
(translation `{0, 4, 9}`, pitched down 24°, perspective `fov = 55`), a ground plane,
and four cubes at distinct transforms — all as plain `SceneObject`s with only a
`TransformComponent` (plus `CameraComponent` on the camera). Each frame,
`onRender` calls `m_scene.cameraUniforms(aspect)` and feeds the result straight into
`renderer.setCamera(cu)` — the renderer's camera comes entirely from the scene's
primary camera, with no separate camera object maintained by the game. The same
`CameraUniforms` are also wrapped in a `MatrixCamera` to construct a `MeshRenderer`
for mesh submission, and each visual's placement is read directly from its
`TransformComponent::localTransform()` at submit time.

## Scope for this slice

- `Scene` owning a single registry; `SceneObject` as a non-owning handle with no
  component data of its own.
- `IDComponent`/`TagComponent`/`TransformComponent`/`CameraComponent` as plain data;
  `TransformComponent` as the sole pose source; `CameraComponent` as projection-only.
- The additive `setCamera(const CameraUniforms&)` overload and the `MatrixCamera`
  bridge, with every existing camera class and game left untouched.
- **Reserved, not implemented:** camera controller components/behaviours (Fly /
  Orbit / Follow / 2D); `MeshComponent` + `Scene::render` (scene-driven geometry
  submission); parent/child transforms and scene traversal/lifecycle callbacks;
  migrating `arena`/`shooter` onto `Scene`; retiring or consolidating the standalone
  camera classes; a general `lookAt`-based transform helper.

## Next Milestones

- Camera controller components (Fly / Orbit / Follow / 2D) driving
  `TransformComponent` each frame.
- `MeshComponent` and a `Scene::render` path for scene-driven geometry submission.
- Parent/child transform hierarchy.
