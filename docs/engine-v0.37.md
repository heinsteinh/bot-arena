# Engine v0.37 — Nearest-N Light Selection + Cascaded Shadow Maps

v0.37 ships two independent renderer improvements. **(A)** point lights are sorted
by camera distance before upload, so the parallax self-shadow (v0.36) covers the
three lights *nearest the camera* instead of the first three by index. **(B)** the
single fixed directional shadow map is replaced with **3 cascaded shadow maps** for
crisp near shadows and covered far shadows. Implementing (B) also uncovered and
fixed a long-standing bug that had disabled the directional shadow (and much of the
directional light) engine-wide. A new `csm_demo` shows the cascades. Design
rationale:
`docs/superpowers/specs/2026-07-16-engine-v0.37-nearest-lights-and-csm-design.md`.

## Part A — Nearest-N light selection

`Renderer::setPointLights` now stores the light list, and `Renderer::endFrame`
sorts a copy by distance to the camera (`m_camera.cameraPosition`) ascending before
uploading the PointLights UBO. Indices 0/1/2 are therefore the three nearest lights,
so the v0.36 geometry pass writes *their* self-shadows into `gShadow.g/b/a` and the
lighting pass attenuates them — with **no shader change**. The point-light sum is
order-independent, so reordering is transparent to lighting; emissive billboards use
the same sorted list. Per-fragment "nearest" would require per-fragment light
indices in the g-buffer (too heavy); nearest-to-camera is the standard, cheap
approximation. Scenes with ≤3 point lights (e.g. `parallax_light_demo`) are
unchanged (nearest 3 == all 3).

## Part B — Cascaded shadow maps

- **3 cascades** replace the single 2048² directional map. `Renderer::endFrame`
  splits the camera frustum by view depth using a practical scheme (uniform/log
  blend, `λ = 0.5`, over `[kShadowNear = 0.5, kShadowFar = 60]`), fits a tight
  light-space ortho to each slice (`makeCascadeViewProj` — maps depth → NDC z with
  the camera projection, takes the slice's frustum-corner centroid + bounding
  radius), and renders scene depth into one of **3 `depthOnly` framebuffers**
  (`m_shadowFBO[3]`).
- **`LightUniforms`** grew to `cascadeViewProj[3]` + `cascadeSplits` (vec4) +
  `lightDir`. This block (binding 1) is read by **both** the lighting shader and
  `mesh.glsl` (for `u_lightDir`), so both Light blocks were updated in lockstep to
  the new std140 layout.
- **`shadowPCF`** now derives the fragment's view-space depth (`-(u_view *
  worldPos).z`), picks the cascade against `u_cascadeSplits`, and 3×3-PCF-samples
  that cascade. Cascade selection uses a **branched** `if/else` over three named
  `sampler2DShadow`s (units 3/10/11) — never dynamic `sampler[]` indexing, which is
  undefined in core GL.
- **`setLight`** takes the 3 cascade depth textures; **`lightingPass`** dropped its
  single `shadowMap` argument (the cascade maps are bound from `m_cascadeMap[3]`).
- **`BOTARENA_CSM=1`** tints each fragment by its chosen cascade (red near / green
  mid / blue far) so the splits are visible for debugging.

Only the directional **macro** shadow (`shadowPCF`) is cascaded; the parallax
directional self-shadow (`gShadow.r`, marched in the geometry pass) is independent
and unchanged.

## Directional-shadow bug fix (engine-wide)

`makeLightViewProj` placed the shadow-camera eye at `center - lightDir * dist`.
Because `lightDir` points *toward* the light (e.g. up), the eye landed on the far
side of the scene (below it), so the shadow map captured **bottom** faces and every
up-facing surface was self-occluded by its own underside — the directional light was
effectively always fully self-shadowed, contributing almost nothing to any scene for
the project's entire history. The cascade fitter places the eye correctly on the
light side (`center + lightDir * dist`, with an up-vector guard for an overhead
sun), so **directional lighting and directional shadows now work for the first
time**.

Consequence: every scene that used the directional light is now brighter and casts
real directional shadows. Two demos tuned around the previously-dark directional
(`normalmap_demo`, `parallax_light_demo`) had their directional light re-tuned to a
low grazing angle so their point-light / relief effects still read; arena and
shooter keep their now-correct, brighter look with real cast shadows. This is an
intentional visual change from the v0.34–v0.36 baselines, not a regression.

## Data flow

```
setPointLights(list) -> Renderer stores m_pointLights                 (A)
endFrame:
  sort m_pointLights by camera distance -> upload UBO                 (A: nearest 3 -> 0/1/2)
  for c in 0..2:                                                      (B)
    fit cascadeViewProj[c] to camera sub-frustum[c]
    render depth into m_shadowFBO[c]
  setLight(cascade maps + splits + dir)
  geometry pass -> g-buffer + gShadow
  lighting pass:
    shadowPCF: pick cascade by view depth -> PCF that cascade         (B)
    directional *= (1 - cascadedShadow) * gShadow.r
    point light i *= (i<3 ? gShadow[i+1] : 1)                         (A: i=0..2 = nearest)
```

## `csm_demo`

`csmdemo::CsmDemoGame` (`csm_demo_game` target): a long ground with a row of pillars
receding into the distance under a low directional sun. Near pillars cast crisp
contact shadows (cascade 0); far pillars stay shadowed (cascades 1/2) — the
before/after a single fixed map cannot do. `BOTARENA_CSM=1` tints the three cascade
regions so the distance-based split is visible.

## Testing

Screenshot-validated (consistent with the rest of the deferred renderer): `csm_demo`
shows crisp near / covered far shadows and, with `BOTARENA_CSM=1`, three distinct
distance-banded cascade regions. Regression: `parallax_light_demo` (3 lights) is
unchanged by Part A; arena/shooter/normalmap render correctly with the now-working
directional light (brighter, with real shadows — an intentional change, see above).
The full build and unit suite (`bot_arena_tests`, 130 cases) stay green.

## Scope for this slice

- **Nearest-N:** nearest 3 point lights by camera distance; folds into the v0.36
  `gShadow.g/b/a` channels.
- **CSM:** 3 cascades, 2048² each, separate `depthOnly` FBOs + branched sampling.
- **Reserved, not implemented:** texture-array cascades (`sampler2DArrayShadow`, one
  sampler, no branch); more than 3 cascades; per-cascade resolution; cascade blend
  bands to hide seams; texel-snapping for shimmer-free movement; per-fragment
  nearest-light selection.

## Next Milestones

- Texture-array cascades + cascade blend bands.
- Per-cascade resolution and stabilized (texel-snapped) cascade fits.
- Nearest-N point-light selection per fragment.
