# Renderer v0.12 — Emissive Point-Light Billboards

v0.12 draws each point light as a bright camera-facing emissive billboard in the
HDR scene after lighting, so the v0.11 bloom pass turns them into glowing orbs. The
lights — previously invisible illuminators — become visible, and bloom gains real
emitters (its threshold rises back to a targeted value).

See `docs/renderer-v0.11.md` for the bloom pass this feeds.

## Frame Data Flow

```
Renderer::endFrame
  shadow → geometry → SSAO → lighting (→ HDR sceneFBO + skybox)
  // --- emissive billboards (new) ---
  (sceneFBO still bound) drawLightBillboards(pointLightCount)   // additive, into HDR
  // --- bloom (threshold now 1.0) ---
  bloom extract/blur → composite(scene + bloom)
```

The billboards write into the same HDR `sceneFBO` the lighting pass just filled, so
they are part of the HDR image the bloom bright-pass reads.

## Emissive billboard shader

Backend-owned, instanced (`glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count)`)
over the unit quad:

- **Vertex** — reads `u_points[gl_InstanceID]` from the `PointLights` UBO
  (binding 2) for the light's position and colour, and the camera right/up axes
  from the view matrix in the Camera UBO (binding 0): `right = vec3(u_view[0][0],
  u_view[1][0], u_view[2][0])`, `up = vec3(u_view[0][1], u_view[1][1],
  u_view[2][1])`. It expands the quad to `center + (local.x*right + local.y*up) *
  u_size` (`u_size = 0.25`) and projects by `u_viewProjection`.
- **Fragment** — `falloff = smoothstep(1, 0, length(local))`; outputs
  `color.rgb * color.a * falloff` (HDR). Under additive blend this reads as a
  glowing disc.

## Backend

`drawLightBillboards(int count)`:

- `glDisable(GL_DEPTH_TEST)`, `glEnable(GL_BLEND)`, `glBlendFunc(GL_ONE, GL_ONE)`
  (additive);
- bind the emissive program, set `u_size`;
- `glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count)` over the unit quad;
- restore `glDisable(GL_BLEND)`.

It reads the already-bound Camera (binding 0) and `PointLights` (binding 2) UBOs;
no new buffers.

## No depth test

The lights ride at `y = 1.5`, above the swarm (cubes ≤ ~0.65) and walls (~1.0), so
they float above the geometry and the billboards are drawn without a depth test
(always on top). Foreground geometry does not occlude them — depth-tested /
soft-particle billboards are a follow-up.

## Renderer

`Renderer` tracks `m_pointLightCount` (set in `setPointLights`, clamped to 32) and
calls `drawLightBillboards(m_pointLightCount)` right after the lighting pass, before
bloom. The bloom bright-pass threshold rises from `0.45` (the v0.11 workaround for
having no emitters) back to `1.0`, so bloom is driven by the light orbs instead of
hazing the whole scene. `BotArenaGame` is unchanged.

## Testing

- No new unit test — a shader + a draw call, with the billboard/falloff math
  in-shader (CPU duplication would break DRY). The existing 36 Catch2 cases stay
  green.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows ~16 coloured glowing orbs on the
  ring above the swarm, each haloed by bloom, with the general scene haze gone.

## Next Milestones

- Depth-occluded / soft-particle billboards (foreground geometry occluding orbs,
  edge fade against surfaces).
- Textured light sprites and HDR flares / anamorphic streaks.
- Light culling (tiled / clustered) to scale the point-light count.
