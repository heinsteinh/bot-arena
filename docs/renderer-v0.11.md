# Renderer v0.11 — Screen-Space Post: SSAO + Bloom

v0.11 adds two screen-space post-processing effects on the deferred/HDR pipeline,
at opposite ends of the lighting pass: **SSAO** (ambient occlusion from the
G-buffer, darkening the IBL ambient) before lighting, and **bloom** (HDR glow)
after lighting, folded into the composite.

See `docs/renderer-v0.10.md` for the deferred IBL pipeline this builds on.

## Frame Data Flow

```
Renderer::endFrame
  shadow  → shadow map
  geometry → G-buffer (albedo/metallic, normal/roughness, worldpos, depth)
  // --- SSAO (new, before lighting) ---
  beginPass(ssaoFBO);      ssaoPass(gNormal, gWorldPos)        → AO raw
  beginPass(ssaoBlurFBO);  ssaoBlur(ssaoFBO.color)             → AO blurred
  setAO(ssaoBlurFBO.color)
  // --- lighting (IBL ambient ×= AO, unit 8) ---
  beginPass(sceneFBO);     lightingPass(...)                   → HDR scene
  // --- bloom (new, after lighting) ---
  beginPass(bloom[0] half); bloomExtract(sceneFBO.color)       → bright (half-res)
  ping-pong bloom[0]/bloom[1]: bloomBlur(src, horizontal) × 10 → bloom final
  // --- composite ---
  beginPass(default);      compositeBloom(sceneFBO.color, bloom final)
```

## SSAO

- **`generateSSAOKernel(count)`** (`engine/renderer/SSAOKernel.hpp`, GL-free): a
  hemisphere sample kernel (`+Z`, within the unit sphere, biased toward the origin),
  deterministic (a fixed hash stands in for `rand`) so it is unit-tested. The
  backend uploads 32 samples to the SSAO shader once at construction, with
  `radius = 0.5`, `bias = 0.025`.
- **`ssaoPass(gNormal, gWorldPos)`** — a fullscreen view-space SSAO: it transforms
  the world-space G-buffer by `u_view`, builds a TBN from the normal + a per-pixel
  hash rotation (no noise texture), samples the kernel, projects each with
  `u_projection`, and accumulates occlusion with a `smoothstep` range check. Writes
  AO (`.r`) to a full-res `RGBA8` buffer.
- **`ssaoBlur(aoRaw)`** — a 4×4 box blur removes the hash noise.
- **`setAO(aoTexture)`** — the lighting pass binds it to unit 8 (`sampler2D u_ao`)
  and multiplies **only the IBL ambient**: `color = (kD*diffuseIBL + specularIBL) *
  ao`. Direct light is unaffected.

## Bloom

- **`bloomExtract(sceneTex)`** — a bright-pass: keeps pixels with luminance above a
  threshold (`0.45`), into a half-res `RGBA16F` buffer.
- **`bloomBlur(src, horizontal)`** — a separable 5-tap Gaussian; the Renderer
  ping-pongs two half-res buffers for 5 horizontal+vertical pairs.
- **`compositeBloom(sceneTex, bloomTex)`** — the final pass: `scene + bloom * 0.6`,
  then Reinhard + gamma. This replaces the old single-texture `blit` composite.

## Renderer

Owns `m_ssaoFBO` + `m_ssaoBlurFBO` (full-res `RGBA8`) and `m_bloomFBO[2]` (half-res
`RGBA16F`); `beginFrame` resizes the SSAO buffers to the window and the bloom
buffers to half. `endFrame` runs the SSAO sub-sequence before lighting and the
bloom sub-sequence + composite after. `BotArenaGame` is unchanged.

## Tuning knobs

- **SSAO:** `radius`, `bias` (backend constructor); the kernel size (32).
- **Bloom:** `u_threshold` (0.45) and `u_bloomIntensity` (0.6) in the backend, and
  `kBloomBlurPasses` (5) in the Renderer. The threshold is low because this scene
  has no compact bright emitters in the default view (the sun is off-frame and the
  point lights are not drawn as geometry), so a lower threshold lets the lit
  surfaces glow visibly.

## Testing

- `test_ssao_kernel` (Catch2, no GL): `generateSSAOKernel` returns hemisphere
  samples (`z ≥ 0`, `length ≤ 1`), is deterministic, and is non-degenerate.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows AO darkening in the swarm's
  crevices / cube-to-wall contacts, and a soft bloom glow on the bright wall tops
  and lit faces — both clearly distinct from v0.10.

## Next Milestones

- Temporal AO / HBAO-GTAO for higher-quality occlusion.
- Karis-average / lens-dirt bloom; exposure / auto-exposure.
- Drawing point lights as emissive billboards (so they bloom directly).
- Light culling (tiled / clustered) to scale the point-light count.
