# Engine v0.30 — SDF Text Effects (Slice 3)

v0.30 consumes the `pxRange` seam v0.29 left unused: the SDF text path now
composites real outline, outer glow, and drop-shadow effects in a single
draw, driven by a per-batch style-table UBO. This replaces `text_demo`'s
app-level layered draws (multiple passes, chunky corners) with one smooth,
scale-correct pass. Design rationale:
`docs/superpowers/specs/2026-07-15-engine-v0.30-sdf-effects-design.md`.

## What's new (`engine/renderer/text/`)

- **`TextStyle` expands** — alongside `fillColor`, it now carries
  `outlineColor`/`outlineWidthPx`, `glowColor`/`glowSizePx`, and
  `shadowColor`/`shadowOffsetPx`/`shadowSoftnessPx`. All effect fields
  default to zero, so a caller that only sets `fillColor` renders exactly as
  in v0.28/29. Widths, sizes, and offsets are in screen pixels. A `styleIndex`
  field exists but is internal — `TextRenderer` assigns it, callers never
  touch it.
- **`GpuStyle`** (`TextStyle.hpp`) is a `std140`-laid-out mirror: 6 `vec4`s
  (`fillColor`, `outlineColor`, `glowColor`, `shadowColor`, `params0` =
  `outlineWidthPx, glowSizePx, shadowOffsetPx.xy`, `params1` =
  `shadowSoftnessPx, pad, pad, pad`), 96 bytes/style. `toGpuStyle(TextStyle)`
  converts one to the other and is unit-tested for field placement.
- **`TextRenderer` builds a per-batch style table** — `Batch` gains
  `std::vector<GpuStyle> styles`. `submit` converts each call's `TextStyle`
  to a `GpuStyle` and dedups by value into the batch's table (`acquire`),
  writing the resulting index into every emitted glyph vertex's
  `styleIndex`. If a batch's table would exceed `kMaxStylesPerBatch` (64), a
  new batch is started for the same `(backend, atlas)` key rather than
  growing past the cap — a documented split, unit-tested at the boundary.
- **Style-table UBO + single-pass effect shader** — `drawTextBatch` now
  takes the batch's `styles` alongside verts/`pxRange`. `OpenGLBackend` owns
  one `UniformBuffer` for the style table, bound at **binding 3**, uploaded
  once per SDF batch and read by the SDF fragment shader as
  `layout(std140, binding = 3) uniform StyleTable { GpuStyle styles[64]; }`.
  The shader resolves a scale-correct px range via `screenPxRange()` (derived
  from `uPxRange` and `fwidth(vUV)`), turns that into a signed screen-px
  distance, and composites back-to-front through a small `over()` helper:
  shadow → glow → outline → fill. The shadow layer samples the distance
  field a second time at a UV offset scaled by `fwidth(vUV)`, so the shadow
  displacement stays correct in screen pixels regardless of draw scale.
  Bitmap batches don't bind or read the table.
- **`drawTextBatch` / `Renderer` forwarding** — the `RenderBackend` and
  `OpenGLBackend` signatures both gained `const std::vector<GpuStyle>&
  styles`; `Renderer::endFrame` forwards `b.styles` alongside the existing
  `(backend, atlas, verts, pxRange)`.
- **SDF spread widened 8 → 24** (`SdfFreeTypeSource`) — outline and glow need
  distance-field range beyond the glyph's own edge to have room to grow into;
  at the old 8 px spread, wider outlines and glow would flood past the
  encoded range and clip. `pxRange` (fed to `screenPxRange`) follows the
  spread, so this is an effects-enabling change, not a visual regression:
  plain fill at spread 24 still resolves to the same 0.5 iso-line.
- **`text_demo`** — the outline/cartoon/stroke/glow/drop-shadow rows now
  build a `TextStyle` with the relevant fields and call `drawText` once per
  label, replacing the old app-level `drawShadow`/`drawOutline` layering
  helpers. The SDF-vs-bitmap comparison from v0.29 is unchanged.

## Scope for this slice

Per the design spec's decisions:

- **Effects:** fill + outline (1 layer) + outer glow + drop-shadow,
  single-pass, SDF-only.
- **Bitmap path, debug HUD, `shooter_game`, and `arena_game` are unchanged**
  — they use plain `TextStyle` (fill only) and render pixel-identically to
  v0.29.
- **Reserved for later:** gradient fill, multiple outline layers (N>1), rich
  text / per-character styles (Slice 4), and MSDF all remain out of scope.

## Testing

`test_text_style.cpp` covers `toGpuStyle` field packing, `GpuStyle`'s 96-byte
size and value equality, and that a default `TextStyle` has all effects off.
`test_text_batch.cpp` covers the style table itself: equal styles dedup to
one entry with a shared `styleIndex`, distinct styles get distinct indices,
and a batch exceeding `kMaxStylesPerBatch` splits. Effect compositing (the
shader math) is validated visually via `text_demo`'s outline/cartoon/stroke/
glow/shadow rows; no GL involvement is added to `bot_arena_tests`. All
existing text tests stay green; bitmap-backed output (HUD/shooter/arena) is
unaffected since none of those callers changed backend or style usage.

## Next Milestones

- Slice 4: rich text (style runs, per-character color/style, markup).
- Slice 5: runtime MSDF (optional `msdfgen`), dynamic atlas, lazy-on-miss
  glyphs.
- Slice 6: world-space 3D text and remaining placement modes.
- Gradient fill and multi-layer outlines, if a future slice needs them.
