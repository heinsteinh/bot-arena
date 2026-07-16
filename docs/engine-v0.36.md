# Engine v0.36 — Point-Light Parallax Self-Shadows

v0.36 extends v0.35's parallax self-shadow — which was computed only for the
directional light and written to `gShadow.r` — to point lights. The geometry pass
now also marches the self-shadow ray toward the first three point lights and packs
the results into the spare `gShadow.g/b/a` channels; the deferred lighting pass
multiplies each of those point lights' contribution by its channel. Raised surface
detail near a point light casts self-shadows into the crevices that sweep as the
light moves. A new `parallax_light_demo` shows a parallax brick floor under a point
light orbiting low. Design rationale:
`docs/superpowers/specs/2026-07-16-engine-v0.36-pointlight-parallax-shadow-design.md`.

## What's new

- **Point-light self-shadow packing.** The existing `RGBA8 gShadow` g-buffer target
  (v0.35) is reused, no new attachment: `.r` = directional self-shadow (unchanged),
  `.g/.b/.a` = the first `min(3, u_pointCount)` point lights, by UBO index. Lights 4+
  get no self-shadow (multiply by 1).
- **Geometry pass (`assets/shaders/mesh.glsl`).** The fragment stage binds the
  **existing PointLights UBO (binding 2)** — already uploaded before the geometry
  pass, since `setPointLights` runs in the game's `onRender`. When `u_hasHeight`,
  after POM finds the parallax UV, it marches the v0.35 `parallaxShadow` toward the
  directional light (→ `.r`) and toward each of the first three point lights:
  ```glsl
  vec4 selfShadow = vec4(1.0);   // r = directional, gba = point lights 0..2
  if (u_hasHeight == 1) {
      // ... POM -> uv ; selfShadow.r = parallaxShadow(uv, dirLightT) ...
      int np = min(u_pointCount, 3);
      for (int i = 0; i < np; ++i) {
          vec3 Lw = normalize(u_points[i].positionRadius.xyz - v_worldPos);
          vec3 Lt = normalize(transpose(TBN) * Lw);
          selfShadow[i + 1] = parallaxShadow(uv, Lt);
      }
  }
  gShadow = selfShadow;
  ```
  `parallaxShadow` is unchanged from v0.35 — it is simply called per light.
- **Non-parallax default is now `vec4(1.0)`.** v0.35 wrote `gShadow = (1,0,0,1)` for
  non-parallax fragments (fine when only `.r` was read). Now that `.g/.b/.a` gate
  point lights, the default must be `vec4(1,1,1,1)` — otherwise point lights 0 and 1
  would be zeroed on every non-parallax surface. This is the one change that touches
  all existing content, and the shooter/arena regression screenshots exist to catch
  it.
- **Lighting pass (`OpenGLBackend` lighting shader).** `u_gShadow` is now sampled as
  a `vec4` (the directional term keeps using `.r`), and the point-light loop
  attenuates the first three lights:
  ```glsl
  vec4 pShadow = texture(u_gShadow, v_uv);   // r=dir, gba=points 0..2
  // ... directional *= (1.0 - pcfShadow) * pShadow.r ...
  for (int i = 0; i < u_pointCount; ++i) {
      // ... radiance / attenuation ...
      float ps = i < 3 ? pShadow[i + 1] : 1.0;
      color += brdf(N, V, Lp, albedo, metallic, rough, F0, radiance) * ps;
  }
  ```
  Point lights 4+ and IBL/ambient are never attenuated by the parallax term. There is
  **no `lightingPass` signature change** — `gShadow` (texture unit 9) was already
  bound and sampled in v0.35; this reads its other channels.

## Data flow

```
mesh + Material{..., heightMap}
  -> executeGeometry: bind height(2); PointLights UBO (binding 2) already uploaded
  -> mesh.glsl geometry pass (u_hasHeight):
       POM -> uv
       parallaxShadow(uv, dirLightT)      -> gShadow.r
       parallaxShadow(uv, pointLightT[i]) -> gShadow.g/b/a   (first 3 points)
     non-parallax -> gShadow = (1,1,1,1)
  -> lighting pass:
       directional *= (1 - pcfShadow) * gShadow.r          (v0.35)
       point light i *= (i < 3 ? gShadow[i+1] : 1.0)       (new)
       IBL/ambient untouched
```

## `parallax_light_demo`

`parallaxlightdemo::ParallaxLightDemoGame` (`parallax_light_demo_game` target) shows a
parallax brick floor (scaled cube, `brick_d.jpg` + `brick_n.jpg` + `brick_h.png`,
`heightScale = 0.04`) with **three distinctly-colored point lights** (warm orange,
cool blue, green) orbiting **low** and close, 120° apart — so all three point-light
channels (`gShadow.g/b/a`) are exercised at once. Each light casts its own parallax
self-shadow, so a crevice shadowed from one light but lit by another shows a colored
self-shadow; the shadows sweep as the cluster orbits — the defining proof this is a
*point-light* self-shadow, not the directional one. The engine draws each point light
as an emissive billboard automatically, so their positions are visible. The
directional light is aimed nearly straight down (flat fill), so the point lights are
the unmistakable casters. `BOTARENA_LIGHT=0|1|2` freezes the cluster at three
orientations for before/after screenshots; the camera looks down at an oblique angle.
`brick_h.png` is the v0.35 asset — no new texture.

## Testing

The GL/shader path is screenshot-validated, consistent with the rest of the deferred
renderer: `parallax_light_demo` across the `BOTARENA_LIGHT` presets shows mortar
self-shadows that sweep to follow the orbiting point light. Regression is covered by
the existing games — their materials have `heightMap == null` → `u_hasHeight == 0`, so
they write `gShadow = vec4(1,1,1,1)` and every light term multiplies by 1. The load-
bearing check is **arena** (four point lights): its colored cubes are pixel-identical
to the v0.35 baseline, confirming the `vec4(1.0)` default does not zero point lights;
shooter is unchanged; `normalmap_demo`'s directional self-shadow still uses `.r`. The
full build and unit suite (`bot_arena_tests`, 130 cases) stay green.

## Scope for this slice

- **Point lights shadowed:** the first `min(3, u_pointCount)` by UBO index; folded
  into the existing `gShadow` target's `.g/.b/.a` channels.
- **Reserved, not implemented:** nearest-N point-light selection (vs first-N); more
  than 3 shadowed point lights (a 2nd `gShadow` target); per-distance / per-radius
  softening of the point-light self-shadow.

## Next Milestones

- Nearest-N point-light selection.
- More than 3 shadowed point lights via a second shadow target.
- Distance-based softening of the parallax self-shadow penumbra.
