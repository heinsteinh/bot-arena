# Renderer v0.10 — Image-Based Lighting (Split-Sum)

v0.10 completes IBL. Three maps are precomputed once from the v0.9 environment
cubemap and sampled per-pixel to replace v0.8's constant ambient with physically
based ambient — diffuse sky fill and specular environment reflections. Metals now
mirror the sky.

See `docs/renderer-v0.9.md` for the environment cubemap this builds on.

## Data Flow

```
Renderer ctor (once, after the env cubemap)
  │  m_irradianceMap = TextureCube::Create(32, 1)
  │  m_prefilterMap  = TextureCube::Create(128, 5)
  │  m_brdfFBO       = Framebuffer::Create({512, 512})           // RGBA16F
  │  backend.convolveIrradiance(env, irradiance, 32)
  │  backend.prefilterEnvironment(env, prefilter, 128, 5)
  │  backend.beginPass(m_brdfFBO, ...); backend.integrateBRDF()
  │  backend.setIBL(irradiance, prefilter, m_brdfFBO->colorAttachment(), 5)
  ▼
Lighting pass (fullscreen)   [binds env 4, irradiance 5, prefilter 6, brdfLUT 7]
  ... skybox (v0.9) + direct Cook-Torrance (v0.8) unchanged ...
  ambient = split-sum IBL:
    F        = fresnelSchlickRoughness(NoV, F0, roughness)
    kD       = (1 - F) * (1 - metallic)
    diffuse  = texture(u_irradiance, N).rgb * albedo
    R        = reflect(-V, N)
    prefilt  = textureLod(u_prefilter, R, roughness * (u_prefilterMips - 1)).rgb
    ab       = texture(u_brdfLUT, vec2(NoV, roughness)).rg
    specular = prefilt * (F * ab.x + ab.y)
    color    = kD * diffuse + specular
```

## The three maps

- **Irradiance cubemap** (`TextureCube` 32², 1 mip) — `convolveIrradiance` renders
  a cosine-weighted hemisphere convolution of the env into the 6 faces (reusing the
  v0.9 per-face fullscreen + `kCubeFaces` basis). Sampled by the surface normal `N`
  for diffuse ambient.
- **Prefiltered env cubemap** (`TextureCube` 128², 5 mips) — `prefilterEnvironment`
  GGX-importance-samples the env at `roughness = mip / (mips-1)` and renders each
  roughness into a cubemap mip (render-to-cubemap-**mip**-face). Sampled by the
  reflection vector `R` at `LOD = roughness * (mips-1)` for specular ambient.
- **BRDF integration LUT** (512² `Framebuffer`, RGBA16F, `.rg`) — `integrateBRDF`
  renders the split-sum scale/bias integral fullscreen. Reuses `Framebuffer`; no new
  2D-texture resource. Sampled by `(NoV, roughness)`.

The shaders use the standard Hammersley + GGX importance sampling; the prefilter
and BRDF programs share those helper functions (duplicated per GLSL program, since
inline shaders cannot `#include`).

## Backend

- `convolveIrradiance(env, irradianceCube, size)`, `prefilterEnvironment(env,
  prefilterCube, baseSize, mipCount)`, `integrateBRDF()` (into the bound FBO), and
  `setIBL(irradiance, prefilter, brdfLUT, prefilterMips)`.
- `lightingPass` binds the three maps to texture units 5/6/7 and sets
  `u_prefilterMips`. The env cubemap (unit 4, v0.9) still drives the skybox.
- The 6-face basis table `kCubeFaces` is shared by `renderEnvironment` and both
  convolution methods.

## Lighting pass

The fragment shader adds `samplerCube u_irradiance`, `samplerCube u_prefilter`,
`sampler2D u_brdfLUT`, `int u_prefilterMips`, and a roughness-aware
`fresnelSchlickRoughness`. The constant-ambient block from v0.8 is replaced by the
split-sum ambient above. The direct-light Cook-Torrance accumulation and the skybox
branch are unchanged.

## Renderer

Owns `m_irradianceMap`, `m_prefilterMap` (`TextureCube`) and `m_brdfFBO`
(`Framebuffer`); generates all three once at init after the env cubemap, then calls
`setIBL`. The maps are static, so `endFrame` is unchanged. `BotArenaGame` does not
change.

## Testing

- No new unit test — the convolution / importance-sampling math is entirely
  in-shader and a CPU copy would duplicate it (consistent with the earlier GPU-only
  pieces). The existing 34 Catch2 cases stay green.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows sky-coloured ambient fill (the
  ground/walls pick up the sky's blue irradiance) and the metal cubes reflecting the
  sky — a clear jump from v0.9's flat ambient.

## Next Milestones

- `.hdr` environment loading (replace the procedural sky) and dynamic env
  re-convolution.
- An AO channel / SSAO feeding the ambient occlusion term.
- Multi-scatter energy compensation, bloom, and light culling (tiled/clustered).
