# Camera System

The renderer is driven by a small, polymorphic camera stack. Cameras describe
*where you look from*; controllers describe *how input moves the camera*. The
two concerns are kept separate so a camera can be reused under different control
schemes (or none at all, e.g. a fixed cinematic shot).

```
Input (state)  ──►  CameraController  ──►  Camera  ──►  Renderer::setCamera
  keyboard            update(dt)            view()        u_viewProjection
  mouse               resize(aspect)        projection()
  scroll                                    viewProjection()
```

## Cameras

All cameras derive from `engine::Camera` (`engine/renderer/Camera.hpp`), an
abstract base with two pure-virtual matrices and one shared helper:

```cpp
class Camera {
 public:
  virtual glm::mat4 view() const = 0;        // world -> camera space
  virtual glm::mat4 projection() const = 0;  // camera space -> clip space
  glm::mat4 viewProjection() const { return projection() * view(); }
};
```

`Renderer::setCamera(const Camera&)` takes the base by reference, so any concrete
camera works polymorphically. The OpenGL renderer caches
`camera.viewProjection()` and uploads it once to the `u_viewProjection` uniform;
every vertex is transformed by it.

| Camera              | Projection    | Use case                              |
| ------------------- | ------------- | ------------------------------------- |
| `PerspectiveCamera` | `perspective` | 3D views (fly, orbit)                 |
| `OrthographicCamera`| `ortho`       | Top-down / isometric world view       |
| `Camera2D`          | `ortho`       | Screen-space UI overlay (pixels)      |

### PerspectiveCamera

Stores position plus a **yaw/pitch** orientation rather than a raw matrix, which
makes it directly drivable by mouse-look. The forward vector is reconstructed
from the two angles using spherical coordinates:

```cpp
forward = normalize(vec3{
  cos(yaw) * cos(pitch),   // x
  sin(pitch),              // y
  sin(yaw) * cos(pitch)    // z
});
```

`right` and `up` are derived with cross products, giving a full orthonormal
basis. `view()` is then `glm::lookAt(position, position + forward, worldUp)`.

`lookAt(position, target)` is the inverse: it back-solves yaw/pitch from a
direction so you can aim the camera at a point and still drive it with angles
afterwards (`pitch = asin(dir.y)`, `yaw = atan2(dir.z, dir.x)`).

### OrthographicCamera / Camera2D

`OrthographicCamera` is a plain `position -> target` look-at with an `ortho`
box; bounds are scaled by aspect ratio each frame so the world isn't stretched.
`Camera2D` maps directly to pixels — origin top-left, `(width, height)`
bottom-right — ready for a future `drawText()` UI layer.

## Controllers

A controller owns a camera and translates per-frame `Input` state into camera
motion. The interface (`engine/renderer/CameraController.hpp`):

```cpp
class CameraController {
 public:
  virtual void update(float dt) = 0;     // apply input for this frame
  virtual void resize(float aspect) = 0; // refresh projection on window resize
  virtual Camera& camera() = 0;          // hand the camera to the renderer
};
```

### FlyCameraController

Free-fly / first-person style.

- **Right mouse drag** rotates (mouse delta × sensitivity feeds yaw/pitch;
  pitch is clamped to ±89° to avoid gimbal flip at the poles).
- **WASD** moves along the *horizontal* forward/right vectors (the `y`
  component is zeroed so looking up doesn't make you fly up), **Q/E** move down/
  up on the world axis.
- Movement is scaled by `dt`, so speed is frame-rate independent.

### OrbitCameraController

Editor / arcball style — the camera circles a fixed target.

- Position is computed on a sphere around the target from `(yaw, pitch,
  distance)`:
  ```cpp
  offset   = vec3{cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw)};
  position = target + offset * distance;
  camera.lookAt(position, target);
  ```
- **Left mouse drag** rotates yaw/pitch around the target (pitch clamped ±89°).
- **Scroll wheel** changes `distance` (dolly zoom), clamped to `[min, max]` so
  you can't pass through or fly infinitely far from the target.

## Input

`engine::Input` (`engine/core/Input.hpp`) is a static snapshot of device state,
populated by the SDL platform layer and consumed by controllers:

- `isKeyDown` / `wasKeyPressed` — held vs. this-frame edge (edge detection is
  used for one-shot actions like toggling camera mode).
- `mouseDelta()` — relative motion since last frame (drives look/orbit).
- `scrollDelta()` — wheel movement this frame (drives zoom).

`Application::run()` calls `Input::beginFrame()` **before** polling events each
frame. That copies the current key state into the previous-frame buffer (for
edge detection) and zeroes the per-frame deltas, so mouse/scroll deltas
represent exactly one frame of movement.

## Wiring it together

`BotArenaGame` holds both perspective controllers plus the top-down camera and
selects between them with a `CameraMode` enum:

```cpp
void BotArenaGame::onUpdate(float dt) {
  if (Input::wasKeyPressed(Key::Space)) cycleCameraMode();  // Fly -> Orbit -> TopDown
  // update only the active controller
}

void BotArenaGame::onRender(Renderer& r, int w, int h) {
  const float aspect = w / (float)h;
  m_flyController.resize(aspect);      // keep projections in sync with the window
  m_orbitController.resize(aspect);
  r.setCamera(/* active camera */);
  // draw the scene...
}
```

## Controls reference

| Mode    | Input             | Action                        |
| ------- | ----------------- | ----------------------------- |
| Fly     | `W A S D`         | Move (horizontal)             |
| Fly     | `Q` / `E`         | Move down / up                |
| Fly     | Right-drag        | Look around                   |
| Orbit   | Left-drag         | Orbit around target           |
| Orbit   | Scroll wheel      | Zoom in / out                 |
| Any     | `Space`           | Cycle Fly → Orbit → TopDown   |
| Any     | `Esc`             | Quit                          |
