# Renderer v0.7 — MRT G-Buffer & Deferred Lighting

v0.7 switches the scene to **deferred shading**. A geometry pass writes per-pixel
material and geometry into a multiple-render-target (MRT) G-buffer; a single
fullscreen lighting pass then reads the G-buffer and computes lighting once per
pixel, so N lights cost O(pixels) instead of O(pixels × objects). The payoff is
shown with one shadowed directional light plus ~16 coloured point lights.

See `docs/renderer-v0.6.md` for the shadow map this builds on.

## Data Flow: One Frame

```
BotArenaGame::onRender
  │  renderer.setCamera(*mainCamera)
  │  renderer.setPointLights(animated ring of ~16 coloured lights)
  │  serial: walls + ground; parallel: swarm            (all Mesh commands)
  ▼
Renderer::endFrame (merge + stable_sort once)
  │  lightViewProj = makeLightViewProj(lightDir, {0,0.5,0}, 16, 0.1, 64)
  │  // pass 0: shadow map (depth only, from the light)
  │  backend.beginPass(shadowFBO 2048², {}, clearDepth, 2048, 2048)
  │  backend.executeShadow(merged, lightViewProj)
  │  backend.setLight({lightViewProj, dir}, shadowFBO->depthAttachment())
  │  // pass 1: geometry -> G-buffer MRT (albedo/normal/world-pos + depth)
  │  backend.beginPass(gbufferFBO, {0,0,0,0}, clearDepth, w, h)
  │  backend.executeGeometry(merged, camera)
  │  // pass 2: lighting -> HDR scene FBO (fullscreen; reads G-buffer + shadow + lights)
  │  backend.beginPass(sceneFBO, {0,0,0,1}, noDepth, w, h)
  │  backend.lightingPass(gAlbedo, gNormal, gWorldPos, shadowMap)
  │  // pass 3: composite -> default framebuffer (tonemap)
  │  backend.beginPass(nullptr, {0,0,0,1}, noDepth, w, h)
  │  backend.blit(sceneFBO->colorAttachment(), fullscreen)
  ▼
saveScreenshot -> readPixels(default framebuffer)
```

The one merged command list drives the shadow pass and the geometry pass; the
lighting and composite passes are fullscreen and read textures.

## MRT Framebuffer

`FramebufferSpec` gained `std::vector<FramebufferFormat> colorFormats` (enum
`RGBA8`/`RGBA16F`). When non-empty, `OpenGLFramebuffer` creates one color texture
per format, attaches `GL_COLOR_ATTACHMENT0 + i`, and calls
`glNamedFramebufferDrawBuffers`. An empty list keeps the previous single-color
behaviour (`hdr ? RGBA16F : RGBA8`). `colorAttachment(uint32_t index = 0)` returns
the i-th attachment. The `depthOnly` shadow path (v0.6) is unchanged.

- **G-buffer:** `{ RGBA8 albedo, RGBA16F world normal, RGBA16F world position }` +
  `DEPTH24_STENCIL8`, window-sized (resized in `beginFrame`).

## Geometry pass — `mesh.glsl` writes the G-buffer

`mesh.glsl` is now a G-buffer writer, not a lighting shader. The vertex shader
emits world position + world normal; the fragment shader writes three MRT outputs:

```glsl
layout(location = 0) out vec4 gAlbedo;    // = u_baseColor
layout(location = 1) out vec4 gNormal;    // = vec4(normalize(worldNormal), 1)
layout(location = 2) out vec4 gWorldPos;  // = vec4(worldPos, 1)
```

`backend.executeGeometry(entries, camera, ...)` uploads the camera UBO, back-face
culls, binds material shaders, and draws each `Mesh` command into the bound MRT.
No shadow-map binding, no line pass.

## Lighting pass

`engine/renderer/PointLight.hpp` — `PointLight { glm::vec4 positionRadius; glm::vec4
color; }` (`.xyz` pos / `.w` radius; `.rgb` colour / `.a` intensity), plus
`pointLightRing(index, count, time, radius, height)` for animated placement.

- `backend.setPointLights(count, lights)` uploads a UBO at **binding 2** laid out
  as `{ int count; PointLight lights[32]; }` (std140; the C++ mirror pads `count`
  to 16 bytes). Count is clamped to 32.
- `backend.lightingPass(gAlbedo, gNormal, gWorldPos, shadowMap)` binds those four
  textures to units 0–3, binds the lighting program + the fullscreen quad, and
  draws (depth test off) into the bound HDR scene FBO.

The lighting shader (backend-owned) reads the `Light` UBO (binding 1) and the
`PointLights` UBO (binding 2) and, per pixel:

- background pixels (no geometry — `gNormal` ≈ 0) shade dark and return;
- ambient `0.15 * albedo`;
- directional: `max(N·L,0) * albedo * (1 - shadow)`, where `shadow` is the 3×3 PCF
  test (projecting `worldPos` by `u_lightViewProj`) that moved here from the mesh
  shader;
- for each point light: `att * max(N·Lp,0) * color.rgb * intensity * albedo`, with
  `att = clamp(1 - d/radius, 0, 1)^2`.

The result is HDR; the existing composite `blit` applies Reinhard + gamma.

## Renderer & game

- `Renderer` owns `m_gbufferFBO` (the MRT) alongside `m_sceneFBO` (the HDR lit
  result) and `m_shadowFBO`. `endFrame` runs shadow → geometry → lighting →
  composite. `setPointLights(const std::vector<PointLight>&)` forwards to the
  backend. The minimap framebuffer/pass/camera were removed.
- `BotArenaGame` builds ~16 coloured `PointLight`s on a slowly rotating ring each
  frame (`pointLightRing`, a 4-colour palette, fixed radius/intensity) and submits
  ground + walls + swarm as meshes. The wireframe grid, debug line, and minimap
  were dropped for this milestone.

The forward render path (`execute` and the line/wireframe renderer) was removed
once the scene switched to deferred.

## Testing

- `pointLightRing` (Catch2, no GL): lights sit on a circle at a fixed height, are
  spaced `2π/count` apart, and the ring rotates rigidly with `time`.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows the swarm + ground rendered
  through the deferred G-buffer, lit by the shadowed directional light and multiple
  coloured point lights (visible coloured light pools), tonemapped.

## Next Milestones

- **Light culling** (tiled / clustered) + light-volume rendering — every point
  light currently shades every pixel (fine at ~16, not at hundreds).
- **Point/spot-light shadows** (cube / perspective shadow maps).
- **SSAO, bloom**, and other G-buffer post-processing.
- **PBR** metallic/roughness materials (more G-buffer channels).
- Returning the grid/minimap as deferred-aware views.
