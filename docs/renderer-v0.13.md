# Renderer v0.13 — ACES Tonemap & Soft Light Billboards

v0.13 is a small polish milestone with two independent final-image improvements:
ACES filmic tonemapping (with an exposure control) replacing Reinhard in the
composite, and soft-particle depth-occluded light billboards replacing v0.12's
always-on-top orbs.

See `docs/renderer-v0.12.md` for the billboards this refines.

## ACES tonemap + exposure

The composite (`compositeBloom`) previously tonemapped `scene + bloom` with Reinhard
(`c/(c+1)`). It now applies a manual exposure and the Narkowicz ACES filmic curve:

```glsl
vec3 aces(vec3 x) {
  const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
// hdr = (scene + bloom * intensity) * u_exposure;  color = pow(aces(hdr), 1/2.2)
```

`u_exposure` is a constant uniform (default `1.0`) set in `compositeBloom`. The
result is punchier contrast and better-preserved highlight colour than Reinhard.
Auto/eye-adaptation exposure is a follow-up.

## Soft / depth-occluded billboards

v0.12's emissive light billboards were drawn always-on-top. They are now
soft-particles, occluded and edge-faded against scene geometry using the G-buffer
world-position — no depth test or depth blit:

- The emissive vertex shader outputs the billboard fragment's world position
  (`v_worldPos`).
- The fragment shader reads the G-buffer world-position at its own pixel
  (`texelFetch(u_gWorldPos, ivec2(gl_FragCoord.xy), 0)`) and compares
  distance-from-camera:
  `fade = smoothstep(0, 0.5, sceneDist - fragDist)`.
  - Behind geometry (`fragDist > sceneDist`) → `fade = 0` (occluded).
  - Near a surface → soft fade (no hard intersection edge).
  - Against the sky (G-buffer `.w == 0`) → `fade = 1`.
- Final colour is `v_color * falloff * fade`.

`drawLightBillboards(int count, uint32_t gWorldPos)` binds the G-buffer world-pos
(RT2) to unit 0; the camera position comes from the Camera UBO (binding 0). The
`Renderer` passes `m_gbufferFBO->colorAttachment(2)`.

Because the demo lights ride above the swarm, the occlusion is subtle in the default
view (visible where orbs graze the swarm), but an orb passing behind a foreground
cube is now correctly hidden.

## Tuning knobs

- `u_exposure` (composite) — overall brightness before the ACES curve.
- The `0.5` fade distance in the emissive fragment shader — how quickly a billboard
  fades as it approaches geometry.

## Testing

- No new unit test — both changes are in-shader (a CPU copy of the ACES curve or the
  fade math purely to test would duplicate shader logic). The existing 36 Catch2
  cases stay green.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows filmic contrast / cleaner
  highlights, and light orbs soft-faded/occluded against the swarm.

## Next Milestones

- **v0.14** — `.hdr` environment loading (replaces the procedural sky for the IBL).
- **v0.15** — light culling (tiled / clustered), scaling the point-light count.
- Also: auto/eye-adaptation exposure, alternative tonemap operators (AgX,
  Uncharted2), textured light sprites, HDR flares.
