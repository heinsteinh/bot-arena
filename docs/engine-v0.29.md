# Engine v0.29 — SDF Text (Slice 2)

v0.29 adds a second text backend on top of the v0.28 text system: a
single-channel signed-distance-field (SDF) path that stays crisp at any
scale, built entirely on FreeType's own outline SDF renderer — no new
dependencies. It is a parallel branch selected by the font's `backend` tag;
the v0.28 bitmap path is unchanged byte-for-byte. Design rationale:
`docs/superpowers/specs/2026-07-15-engine-v0.29-sdf-text-design.md`.

## What's new (`engine/renderer/text/`)

- **`SdfFreeTypeSource`** — a second `FontSource` (`backend() ==
  FontBackend::SDF`). Sets the FreeType `sdf` module's `spread` property to 8
  px (`FT_Property_Set(lib, "sdf", "spread", ...)`), then for each codepoint
  loads the outline (`FT_Load_Char`) and rasterizes it with
  `FT_RENDER_MODE_SDF`. Size/bearing/advance are read from the SDF bitmap's
  own metrics (not the coverage bitmap), since the SDF render pads the glyph
  by the spread. Packs into the same R8 atlas via the unchanged `ShelfPacker`
  and `BakedFont` — SDF is single-channel, so no atlas format change was
  needed. Sets `out.pxRange = spread`. Glyphs with no outline (space) skip
  rendering and keep advance-only, matching the bitmap source's behavior.
- **Backend-aware `drawTextBatch`** — `RenderBackend::drawTextBatch` now takes
  `(FontBackend, atlasTextureId, verts, pxRange)`. `OpenGLBackend` compiles a
  second program, `m_sdfTextProgram`, sharing the v0.28 text VAO/vertex
  shader; its fragment shader resolves coverage from the distance field
  instead of sampling it directly:
  ```glsl
  float d = texture(uAtlas, vUV).r;   // 0.5 == glyph edge
  float w = fwidth(d);
  float a = smoothstep(0.5 - w, 0.5 + w, d);
  ```
  `fwidth`-based `smoothstep` gives scale-adaptive anti-aliasing around the
  0.5 iso-line, so edges stay clean whether the text is drawn small or
  blown up. `drawTextBatch` picks the program by `backend` and, for `SDF`,
  sets a `uPxRange` uniform — reserved for the Slice 3 effects, unused by
  this shader beyond being passed through. Blend mode, depth state, VAO, and
  upload path are identical to the bitmap draw.
- **`TextRenderer::Batch::pxRange`** — batches now carry a `pxRange` field,
  set from `font.pxRange` when a batch is created (constant per
  `(backend, atlas)`, so it does not change the batch key). `Renderer::endFrame`
  forwards `(batch.backend, batch.atlas, batch.verts, batch.pxRange)` into
  `drawTextBatch`.
- **`Renderer` wiring** — `initBuiltins` registers `SdfFreeTypeSource`
  alongside `BitmapFreeTypeSource`; the existing R8 atlas factory is reused
  as-is for both backends.
- **`text_demo`** — adds a side-by-side SDF-vs-bitmap comparison: the same
  short string rendered large (bitmap upscaled 3x vs. an SDF face baked at 48
  px shown at 2x), so the bitmap blur and the SDF crispness are visible in
  the same frame. The SDF face is loaded with `FontDesc{ backend = SDF }`.

## Scope for this slice

Per the design spec's decisions, this is **single-channel FreeType SDF,
demo-only adoption**:

- Only `text_demo` opts into the SDF backend. The debug HUD, `shooter_game`,
  and `arena_game` all keep loading fonts with the default `Bitmap` backend
  and render pixel-identically to v0.28/v0.27.
- **MSDF** (`msdfgen`, multi-channel, sharper corners) is **not**
  implemented — `msdfgen` isn't in the vcpkg manifest or installed in this
  environment, so FreeType's built-in SDF renderer was used instead. MSDF
  remains a reserved, optional later slice.
- **Effects** (outline, cartoon-stroke, glow, drop-shadow, gradient via a
  style-table UBO) are **not** implemented here either; they are still
  Slice 3. This slice ships crisp single-channel fill plus the `pxRange`
  seam those effects will consume — the uniform is wired through end to end
  but the current shader only uses it as a pass-through, not as an input to
  any effect math.
- Unicode decoding, kerning/shaping, and world-space text remain out of
  scope, unchanged from v0.28.

## Testing

`test_text_batch.cpp` gained a case asserting that a submitted font's
`backend` and `pxRange` are recorded on the resulting `Batch` (stub font,
non-zero `pxRange`, no GL/FreeType involved). `SdfFreeTypeSource::bake` is
not unit-tested, matching `BitmapFreeTypeSource` — FreeType stays out of the
`bot_arena_tests` link — and is instead validated visually via the demo's
SDF-vs-bitmap comparison. All existing text tests stay green; HUD/shooter/
arena output is unaffected since none of them changed backend.

## Next Milestones

- Slice 3: single-pass effects (outline/cartoon-stroke/glow/shadow/gradient)
  driven by the style-table UBO, consuming `pxRange` for real.
- Slice 4: rich text (style runs, per-character color, markup).
- Slice 5: runtime MSDF (optional `msdfgen`), dynamic atlas, lazy-on-miss
  glyphs.
- Slice 6: world-space 3D text and remaining placement modes.
