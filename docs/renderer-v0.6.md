# Renderer v0.6 — Directional Shadow Maps

v0.6 adds real shadows. A shadow pass renders scene depth from the light's
orthographic POV into a depth-only texture; the scene pass samples it with 3×3
PCF. A ground-plane mesh receives the swarm's shadows. Shadow mapping is another
render-to-texture consumer built on the v0.5 pass model.

See `docs/renderer-v0.5.md` for the render passes this builds on.

## Data Flow: One Frame

```
BotArenaGame::onRender
  │  renderer.setCamera(*mainCamera); renderer.setMinimapCamera(m_minimapCam)
  │  serial: grid + walls + ground plane; parallel: swarm
  ▼
Renderer::endFrame (merge + stable_sort once)
  │  lightViewProj = makeLightViewProj(lightDir, {0,0.5,0}, 16, 0.1, 64)
  │  // pass 0: shadow map (depth only, from the light)
  │  backend.beginPass(shadowFBO 2048², {}, clearDepth, 2048, 2048)
  │  backend.executeShadow(merged, lightViewProj)
  │  backend.setLight({lightViewProj, dir}, shadowFBO->depthAttachment())
  │  // pass 1: minimap  -> execute(merged, topDownCamera)   [samples shadow map]
  │  // pass 2: scene     -> execute(merged, mainCamera)      [samples shadow map]
  │  // pass 3: composite -> blit(scene) + blit(minimap)
  ▼
saveScreenshot -> readPixels(default framebuffer)
```

The one merged command list drives three scene renders (shadow, minimap, scene).

## Depth-only Framebuffer

`FramebufferSpec` gained `bool depthOnly`. A depth-only `OpenGLFramebuffer`:

- creates a `DEPTH_COMPONENT32F` depth texture (no color), `GL_LINEAR`,
  `CLAMP_TO_BORDER` with a white border (outside the light frustum = lit), and
  `GL_TEXTURE_COMPARE_MODE = GL_COMPARE_REF_TO_TEXTURE` / `GL_LEQUAL` so it can be
  read as a `sampler2DShadow` with hardware depth comparison;
- attaches it as `GL_DEPTH_ATTACHMENT` and sets `glNamedFramebufferDrawBuffer(
  GL_NONE)` / read `GL_NONE`.

`depthAttachment()` returns the depth texture id (the shadow map).

## Light & Light-Space Matrix

`engine/renderer/LightUniforms.hpp`:

```cpp
struct LightUniforms { glm::mat4 lightViewProj; glm::vec4 lightDir; };  // binding 1
glm::mat4 makeLightViewProj(lightDir, center, halfExtent, near, far);
```

`makeLightViewProj` = `ortho(-h,h,-h,h,near,far) * lookAt(center -
normalize(dir)*(far*0.5), center, {0,1,0})` — an orthographic view from behind the
scene along the light direction. The `Renderer` holds the light direction (default
`normalize(0.4,0.8,0.3)`, `setLightDirection` to change) and fixed scene bounds
(`center (0,0.5,0)`, `halfExtent 16`, `near 0.1`, `far 64`).

## Backend

- **`executeShadow(entries, lightViewProj, ...)`** — binds a depth-only shadow
  program (`gl_Position = lightViewProj * transform * pos`, empty fragment), sets
  `u_transform` per `Mesh` command, and `glDrawElements` into the bound depth
  target. Non-mesh commands are skipped (lines don't cast).
- **`setLight(light, shadowMapTexture)`** — uploads the Light UBO (binding 1) and
  remembers the shadow map texture.
- **`execute`** (modified) — binds the shadow map to texture **unit 1** and sets
  `u_shadowMap = 1` on the mesh shader before the mesh pass. Everything else
  (camera UBO, mesh/line passes) is unchanged.

The Light UBO and shadow-depth program are created in the backend constructor.

## `mesh.glsl` (PCF shadows)

The vertex shader emits the world position projected into light space
(`v_lightClip = u_lightViewProj * world`). The fragment shader:

- perspective-divides `v_lightClip` and maps to `[0,1]`;
- applies a slope-scaled bias `max(0.0025*(1-N·L), 0.0008)` to reduce shadow acne;
- does **3×3 PCF**: nine `sampler2DShadow` taps offset by `1/2048`, averaged (each
  tap is a hardware depth comparison, so edges are smooth);
- lights with `baseColor * (ambient + (1 - shadow) * max(N·L, 0))` using the
  light direction from the Light UBO.

## Ground Plane

`BotArenaGame` submits a flat receiver mesh (the unit cube scaled `(20,0.05,20)` at
`y=-0.05`, spanning 40×0.1×40, grey material) so the swarm's shadows land on a
visible surface. The wireframe grid stays as context; cubes also shadow each other
and the walls.

## Testing

- `makeLightViewProj` (Catch2, no GL): the scene `center` maps to the middle of
  light clip space (NDC XY ≈ 0) at a valid depth; a point moved toward the light
  gets a smaller (nearer) light-space depth than one moved away.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows the swarm casting soft PCF
  shadows onto the ground plane and each other, lit from the directional light,
  with the minimap inset still present.

## Next Milestones

- **Multiple lights** (a lights array / multiple shadow maps) and **point/spot
  shadows** (cube / perspective shadow maps).
- **Cascaded shadow maps** for large scenes, a shadow atlas, MRT G-buffer +
  deferred lighting, and normal/roughness/PBR materials.
