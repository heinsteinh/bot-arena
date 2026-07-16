# Engine v0.35 — Parallax Occlusion Mapping + Self-Shadowing

v0.35 adds Parallax Occlusion Mapping (POM) to the deferred g-buffer geometry pass,
on the tangent-space seam v0.34 built. When a material carries a height map, the
mesh fragment stage ray-marches the height field in tangent space and shifts the
texture coordinate **before** sampling albedo and normal — so a single quad gains
true parallax depth and self-occlusion, and the normal is sampled at the displaced
UV so relief and displacement reinforce each other. A second short ray marched
toward the light produces a soft self-shadow, carried through a new 4th g-buffer
target and consumed by the directional light in the deferred lighting pass. A
`normalmap_demo` third wall shows `flat | normal | normal+parallax` under a grazing
directional light. Design rationale:
`docs/superpowers/specs/2026-07-16-engine-v0.35-parallax-mapping-design.md`.

## What's new

- **`Material.heightMap` + `Material.heightScale`** (`ResourceRegistry.hpp`) —
  an optional `Ref<Texture2D>` (default `nullptr`) and a `float` displacement
  depth (default `0.05`). Both are appended at the end of the aggregate, so every
  existing `registerMaterial({...})` call keeps compiling and behaving identically
  (no height map → no parallax). The height map is grayscale, **white = raised**;
  in-shader the marched `depth = 1 - texture(...).r`.
- **POM in the g-buffer geometry pass (`assets/shaders/mesh.glsl`).** The fragment
  stage builds the TBN from the geometric vertex normal (as in v0.34), transforms
  the view vector into tangent space (`transpose(TBN) * (u_cameraPos.xyz -
  v_worldPos)`; the Camera UBO is now bound in the fragment stage), and ray-marches:
  ```glsl
  vec2 parallaxOcclusion(vec2 uv, vec3 viewT) {
      float n = mix(32.0, 8.0, clamp(abs(viewT.z), 0.0, 1.0));  // layers by angle
      // steep march until the height profile crosses the ray, then interpolate
      // the last two layers for a smooth intersection ...
  }
  ```
  The shifted UV feeds both the albedo and the normal sample. The whole block is
  gated on `u_hasHeight`; the height map binds on **texture unit 2** in
  `executeGeometry` (albedo 0, normal 1, height 2).
- **Parallax self-shadow via a 4th g-buffer target `gShadow`** (`RGBA8`,
  attachment 3). The framebuffer impl is already generic over N color attachments,
  so this is one added format in `gbufferSpec.colorFormats`. In the geometry pass,
  after POM finds the parallax UV, a second short ray is marched toward the light
  (`u_lightDir` read from the **existing** Light UBO, binding 1 — already uploaded
  before the geometry pass, so no new plumbing), accumulating a soft occlusion term
  written to `gShadow.r`. **Every geometry fragment writes `gShadow`** (`1.0` when
  there is no height map), so the 4th draw buffer is always defined and non-parallax
  materials stay fully lit.
- **Lighting pass consumes `gShadow`** (`OpenGLBackend` lighting shader, bound on
  **texture unit 9**). Only the **directional light's** contribution is multiplied
  by it:
  ```glsl
  float pShadow = texture(u_gShadow, v_uv).r;
  color += brdf(N, V, L, albedo, metallic, rough, F0, vec3(3.0))
           * (1.0 - shadow) * pShadow;   // shadow = PCF; pShadow = parallax
  ```
  Point lights and IBL/ambient are never attenuated by the parallax term — this is
  correct per-light. `lightingPass` gains a fifth `gShadow` argument across
  `RenderBackend`, `OpenGLBackend`, and the `Renderer::endFrame` call (passing
  `colorAttachment(3)`).
- **The directional light is the self-shadow caster** — the same "sun" that already
  drives the PCF shadow map now shadows at both scales: macro (walls → ground via
  the PCF shadow map) and micro (bricks → mortar via parallax self-shadow).

## Data flow

```
mesh (pos/normal/uv/tangent) + Material{albedo, normalMap, heightMap, heightScale}
  -> executeGeometry: bind albedo(0)/normal(1)/height(2), set u_hasHeight/u_heightScale
  -> mesh.glsl geometry pass:
       TBN; view & light in tangent space (u_cameraPos / u_lightDir UBOs)
       POM shifts uv -> sample albedo + normal at shifted uv
       parallaxShadow -> gShadow.r   (1.0 when no height map)
  -> gAlbedo / gNormal / gWorldPos / gShadow
  -> [unchanged] SSAO (reads attachments 1 & 2)
  -> lighting pass: directional term *= (1 - pcfShadow) * gShadow.r; points + IBL untouched
  -> bloom -> composite (ACES + gamma)
```

## `normalmap_demo`

The demo gains a **third wall**, so it now shows the full progression on identical
brick under one grazing light:

- **flat** — `Material{ albedo = brick_d.jpg }`.
- **normal** — `+ normalMap = brick_n.jpg` (flat-lit relief).
- **parallax** — `+ heightMap = brick_h.png`, `heightScale = 0.08` (real depth +
  self-shadow).

`assets/textures/brick_h.png` is generated from `brick_d` luminance (bricks read
raised, mortar recessed) and committed as a demo asset. The **directional light**
(`setLightDirection`) grazes the walls — driving both the PCF shadow map and the
parallax self-shadow — with a dim point light as fill so the key reads.
`BOTARENA_LIGHT=0|1` freezes two raking angles whose lateral direction flips, so the
self-shadows swing to the opposite side of the bricks between shots. The camera is
oblique (parallax depth reads best off dead-on), with three camera-facing billboard
labels (v0.33 world text).

Visual proof: the parallax wall shows genuinely raised, rounded bricks with recessed
mortar and soft self-shadows in the crevices that shift with the light angle; the
normal wall shows flat-lit relief without occlusion; the flat wall is plain brick.

## Testing

The GL/shader path is screenshot-validated, consistent with the rest of the deferred
renderer (no unit tests for the g-buffer / PCF / SSAO passes): `normalmap_demo` at
both light presets confirms the parallax wall has depth + self-shadowing the
normal-only wall lacks, and that both respond to the light angle; the POM UV-shift
direction (height convention) and the self-shadow strength were tuned in this step.
Regression is covered by the existing games — their materials have
`heightMap == nullptr` → `u_hasHeight == 0`, POM is skipped, and every fragment
writes `gShadow = 1.0`, so the directional light multiplies by 1.0 and nothing
changes; SSAO and the emissive pass read attachments 1 & 2, unaffected by the new
attachment 3. Shooter and arena screenshots are pixel-identical to the v0.34
baselines; the full build and unit suite (`bot_arena_tests`, 130 cases) stay green.

## Scope for this slice

- **Algorithm:** Parallax Occlusion Mapping (steep march + interpolation); the
  height map is a dedicated grayscale texture.
- **Self-shadow:** correct per-light, for the **directional light only**; carried
  through the 4th g-buffer target and folded into the directional term.
- **Reserved, not implemented:** parallax silhouette edge-clipping (`discard` when
  the shifted UV leaves `[0,1]`); per-light parallax self-shadow for point lights;
  packing height into the normal map's alpha channel (one fewer bind); per-material
  POM layer counts and self-shadow strength (currently shader constants).

## Next Milestones

- Silhouette edge-clipping for parallax at grazing angles.
- Point-light parallax self-shadows.
- Packed height in the normal map's alpha; per-material POM tuning.
