# Engine v0.39 — Camera Controller Components

v0.39 gives the v0.38 scene layer its camera *behaviours*: four data-oriented
controller components (Fly, Orbit, Follow, 2D) plus matching systems that read
the `Input` global and `dt`, and write nothing but a camera object's
`TransformComponent` — the single pose source v0.38 established stays single.
As part of wiring look-at orientation robustly, `TransformComponent.rotation`
moves from an euler `vec3` to a `glm::quat`. A new `controller_demo` cycles
through all four. Design rationale:
`docs/superpowers/specs/2026-07-17-engine-v0.39-camera-controllers-design.md`.

## Quaternion transforms

`TransformComponent.rotation` (`engine/scene/Components.hpp`) is now a
`glm::quat`, defaulting to the identity `{1.0f, 0.0f, 0.0f, 0.0f}` (w, x, y,
z). `localTransform()` composes `T * R * S` via
`glm::translate(...) * glm::mat4_cast(rotation) * glm::scale(...)`, and
`viewMatrix` (`engine/scene/SceneCamera.hpp`) inverts
`translate(t.translation) * mat4_cast(t.rotation)` — both call sites just
swapped `glm::quat(rotation)` for `mat4_cast(rotation)` directly, since the
field is already a quat. Nothing else on `TransformComponent` changed:
`translation` and `scale` are still plain `vec3`.

The migration touched only the two v0.38 call sites that set `rotation`
directly: `games/scene_demo/SceneDemoGame.cpp` (now builds a quat from Euler
angles for its pitched-down camera) and `tests/test_scene.cpp`. The
`localTransform` test in `tests/test_scene.cpp` also gained an independent
oracle beyond comparing against a hand-built `T*R*S`: it rotates a unit
`+X` vector 90° about `+Y` through `localTransform()` and asserts the result
lands on `-Z` (with the configured `x`-scale of 2 reflected in length), which
resolves the earlier tautology note from the v0.38 doc — a real basis-vector
check independent of the production formula.

## `CameraMath.hpp`

`engine/scene/CameraMath.hpp` is a new header-only, pure-function module — no
state, `<glm/gtc/quaternion.hpp>` only (no `gtx`) — providing the orientation
building blocks every controller system shares:

- `orientationFromYawPitch(yawDeg, pitchDeg)` — `angleAxis(yaw, +Y) *
  angleAxis(pitch, +X)`. `yaw = pitch = 0` looks down `-Z`; positive yaw turns
  the forward vector off `-Z` about world `+Y`; positive pitch pitches about
  local `+X` and looks *up* (`forwardDir(...).y > 0`), matching a standard
  FPS convention.
- `forwardDir(q)` — the camera forward, `q * (0, 0, -1)`.
- `lookRotation(forward, up = {0,1,0})` — builds an orientation whose `-Z`
  points along `forward` via `glm::quat_cast(glm::mat3(r, u, -f))`, with a
  guard: if `forward` is parallel to `up` (cross product length `<= 1e-5`),
  it falls back to `right = (1, 0, 0)` instead of producing a degenerate
  basis.
- `orbitPosition(center, yawDeg, pitchDeg, distance)` — `pitchDeg` is the
  **elevation above the target**: positive pitch places the camera *above*
  the target, looking down. Internally this negates pitch before reusing
  `orientationFromYawPitch`/`forwardDir` (`orbitPosition` computes
  `orientationFromYawPitch(yawDeg, -pitchDeg)`), because
  `orientationFromYawPitch`'s own pitch sign looks up, and orbit elevation is
  the opposite sense. This is a fix made mid-slice: the first cut reused
  positive pitch directly and put the default `OrbitControllerComponent`
  camera underground; `orbitPosition` was corrected to negate pitch, and the
  orbit system now derives `rotation` from `lookRotation(center - position)`
  rather than reusing `yaw`/`pitch` directly, so the visible look direction
  always faces the orbit target regardless of the position formula's
  internal sign convention.

All four conventions are pinned by `[cameramath]` cases in
`tests/test_scene.cpp`: `orientationFromYawPitch` sign-of-turn checks,
`lookRotation` matching `glm::lookAt`, and
`orbitPosition sits distance from center with positive pitch above`, which
asserts both the distance-from-center invariant and that `pitchDeg > 0`
elevates the camera above `center.y`.

## Controller components

`engine/scene/ControllerComponents.hpp` defines four plain-data components,
one per behaviour, added to `entt` alongside a `TransformComponent`:

- `FlyControllerComponent { moveSpeed = 6.0f; lookSensitivity = 0.08f; yaw =
  -135.0f; pitch = -30.0f; }`.
- `OrbitControllerComponent { target = entt::null; targetPoint = {0, 0.5, 0};
  yaw = 45.0f; pitch = 30.0f; distance = 14.0f; minDistance = 3.0f;
  maxDistance = 40.0f; rotateSpeed = 0.25f; zoomSpeed = 1.5f; }` — orbits
  `target`'s translation if it's a valid entity with a `TransformComponent`,
  else `targetPoint`.
- `FollowControllerComponent { target = entt::null; offset = {0, 4, 9};
  damping = 0.0f; }` — `target` is required (the system no-ops without a
  valid one); `damping = 0` is a hard follow, `> 0` exponentially smooths
  toward the goal.
- `Camera2DControllerComponent { panSpeed = 8.0f; zoomSpeed = 1.5f; }` — a
  top-down ortho controller: looks down `-Y`, pans the ground plane, zooms
  via `orthoSize`.

Orbit is the one behaviour that can target either another entity or a fixed
point, falling back to `targetPoint` when `target` isn't a valid entity —
matching how the old standalone `OrbitCameraController` could orbit a
moving target. Follow always requires a valid entity `target`; its system
simply no-ops (leaves the transform untouched) when one isn't set.

## Systems + `Scene::update(dt)`

`engine/scene/CameraControllerSystems.hpp/.cpp` implements one free function
per controller, each iterating `registry.view<TransformComponent,
XControllerComponent>()`, reading the `Input` global plus `dt`, and writing
only that entity's `TransformComponent` (and, for 2D, also
`CameraComponent::orthoSize` — the one explicitly allowed exception to the
single-pose-source rule, since zoom is a projection parameter, not a pose):

- `updateFlyControllers` — right-mouse-drag updates `yaw`/`pitch` (pitch
  clamped to ±89°); WASD/QE moves `translation` along the yaw-planar
  forward/right/up; `rotation = orientationFromYawPitch(yaw, pitch)`.
- `updateOrbitControllers` — left-mouse-drag updates `yaw`/`pitch` (same
  clamp); scroll adjusts `distance` (clamped to `[minDistance,
  maxDistance]`); `center` resolves to `target`'s translation or
  `targetPoint`; `translation = orbitPosition(center, yaw, pitch, distance)`;
  `rotation = lookRotation(center - translation)`.
- `updateFollowControllers` — no-ops if `target` is invalid or lacks a
  `TransformComponent`; otherwise `goal = targetTranslation + offset`,
  `translation` snaps to `goal` (or, if `damping > 0`, exponentially
  smooths via `glm::mix` with `1 - exp(-damping * dt)`); `rotation =
  lookRotation(targetTranslation - translation)`.
- `updateCamera2DControllers` — WASD pans `translation` in the world `X`/`Z`
  ground plane (`panSpeed * dt`); scroll adjusts `CameraComponent::orthoSize`
  (floored at `0.5`); `rotation` is fixed to a top-down look via
  `lookRotation({0, -1, 0}, {0, 0, -1})`.

`Scene::update(float dt)` (`engine/scene/Scene.hpp/.cpp`) runs all four
systems in order — Fly, Orbit, Follow, Camera2D — against the scene's
registry. A camera entity only moves if it carries the matching controller
component, so attaching exactly one keeps a single well-defined pose per
frame; `controller_demo` relies on this by swapping which controller
component is attached to its one camera entity rather than running several
at once.

## Unit testing

Both the pure math and the `Input`-driven systems are unit-tested — the
systems are testable directly because `engine::Input` exposes public setters
(`setKey`, `setMouseButton`, `setMouseDelta`, `setScrollDelta`) alongside its
query methods, so a test can drive input state without a real window or
event loop. `tests/test_scene.cpp` grew from 11 to 20 cases (`[scene]` +
`[cameramath]` + `[controller]`):

- `[cameramath]`: `orientationFromYawPitch` sign conventions, `lookRotation`
  matching `glm::lookAt`, `orbitPosition`'s distance and elevation
  invariants.
- `[controller]`: orbit pose-from-params with no input (asserts both
  position and that `rotation`'s forward points at the orbit center); orbit
  left-drag changing yaw; fly `W` moving along the planar forward; follow
  sitting at `target + offset` facing the target; 2D panning with keys and
  zooming with scroll.

Every input-driven case brackets its state with `engine::Input::beginFrame()`
before setting input and an explicit key/button release plus a second
`beginFrame()` after, so one test's simulated input can't leak into the
next — a test-isolation pattern flagged as a risk in an earlier slice and
honored here. `engine/core/Input.cpp` is linked into `bot_arena_tests` (added
in v0.38) so the `Input` statics resolve. The full suite grew from 141 to 150
cases (50728 assertions).

## `controller_demo`

`controllerdemo::ControllerDemoGame` (`controller_demo_game` target,
`games/controller_demo/ControllerDemoGame.cpp`) builds one `Scene`: a ground
plane, four cubes, and a single camera object that carries exactly one
controller component at a time. `setController(index)` removes whichever of
the four controller components is currently attached and adds the next,
seeding controller-specific defaults so each behaviour frames the cube
cluster sensibly (e.g. Fly gets an explicit starting translation since it
never repositions itself except via WASD; 2D switches the camera to
orthographic with a widened near/far clip range for its overhead position).
`onUpdate` calls `Input::wasKeyPressed(Key::Space)` to cycle to the next
controller (skipped when the `BOTARENA_SCREENSHOT` env var is set, for
deterministic headless capture) and then `m_scene.update(dt)`. Setting
`BOTARENA_CTRL=0..3` at startup selects Fly/Orbit/Follow/2D directly, for
per-controller reference screenshots. `onRender` reads
`m_scene.cameraUniforms(aspect)` and feeds it straight to
`renderer.setCamera(cu)`, unchanged from the `scene_demo` pattern — the demo
never maintains a separate camera object.

## Scope for this slice

- `TransformComponent.rotation` as `glm::quat` (identity default);
  `localTransform`/`viewMatrix` via `mat4_cast`.
- `CameraMath.hpp`: `orientationFromYawPitch`, `forwardDir`, `lookRotation`,
  `orbitPosition`, gtc-only, with the orbit-elevation convention fixed
  mid-slice.
- The four controller components (`FlyControllerComponent`,
  `OrbitControllerComponent`, `FollowControllerComponent`,
  `Camera2DControllerComponent`) and their systems in
  `CameraControllerSystems.hpp/.cpp`; `Scene::update(dt)` running all four;
  `TransformComponent` as the sole pose source (`CameraComponent::orthoSize`
  is the one explicit projection-parameter exception, for 2D zoom).
- `controller_demo` cycling the four behaviours.
- **Reserved, not implemented:** Follow damping curves/spring beyond the
  simple exponential smoothing, camera momentum/inertia, collision-aware
  orbit; gamepad input; per-controller FOV/projection presets; retiring the
  standalone `FlyCameraController`/`OrbitCameraController`/
  `CameraController` classes and migrating `arena`/`shooter`/`camera_demo`
  onto scene controllers; a `Scene::update` system-registration mechanism for
  user-defined systems instead of the fixed built-in list of four.

## Next Milestones

- Retire or consolidate the standalone camera controller classes once the
  remaining games migrate onto scene controllers.
- `MeshComponent` and a `Scene::render` path for scene-driven geometry
  submission (still reserved from v0.38).
- Parent/child transform hierarchy.
