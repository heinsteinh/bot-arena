# Engine v0.28 — Text System Foundation

v0.28 replaces the single bitmap-atlas text path from v0.17 with a modular,
backend-agnostic text system: a central `FontManager` cache, a backend-tagged
`FontAsset`, a codepoint-keyed `GlyphAtlas`, a surface-agnostic `TextLayout`, a
shared `TextStyle`, and a batching `TextRenderer`. This slice is deliberately
**behavior-preserving** — same bitmap glyphs, same pixels on screen — but every
seam needed for SDF/MSDF, effects, and world-space text is now in place. Full
design rationale: `docs/superpowers/specs/2026-07-15-engine-v0.28-text-system-architecture-design.md`.

## Pipeline

```
FontDesc{family, pixelSize, backend, ...} --FontManager.load()--> FontHandle (cached)
                          |
              (cache miss) BitmapFreeTypeSource.bake() -> BakedFont (R8 pixels + GlyphStore)
                          |
                    atlasFactory -> GlyphAtlas (Texture2D + ShelfPacker dims)

renderer.drawText(font, text, TextPlacement, TextStyle)
  -> TextRenderer::submit(): layoutText() -> local TextQuads
                              placement -> screen px -> NDC
                              emit TextVertex{pos, uv, fillColor, outlineColor, styleIndex}
                              batched by (backend, atlas)
  -> endFrame(): backend.drawTextBatch(atlas, verts) per batch, after compositeBloom
```

## Components (`engine/renderer/text/`)

- **`FontDesc` / `FontDescHash`** — normalized descriptor (family, pixelSize,
  backend, source policy, glyph range, reserved dpiScale/renderTarget) used as
  the `FontManager` cache key, so identical requests share one atlas.
- **`FontManager`** — sole owner/cache. `registerSource(Scope<FontSource>)` per
  backend; `load(FontDesc)` returns a cached `FontHandle` or bakes via the
  registered source and builds the atlas through an injected `AtlasFactory`
  (keeps the manager free of GL; tests inject a stub).
- **`FontAsset` / `FontHandle`** — backend-tagged handle (`Ref<FontAsset>`):
  `backend`, `Ref<GlyphAtlas>`, `GlyphStore`, `FaceMetrics`, `pxRange`
  (0 for bitmap), and an empty `fallback` chain this slice. No `Font` base
  class or per-glyph virtual dispatch — batching stays cheap.
- **`GlyphAtlas` + `ShelfPacker`** — atlas owns a `Texture2D` and dimensions;
  `ShelfPacker` is a row/shelf packer shared by static (this slice) and future
  dynamic atlases.
- **`Glyph` / `GlyphStore`** — `unordered_map<char32_t, Glyph>` (size, bearing,
  advance, atlas UVs, a reserved `glyphIndex`), replacing the old fixed
  `std::array<Glyph,128>`.
- **`FontSource` / `BitmapFreeTypeSource`** — `FontSource` is the provider
  interface (`bake(FontDesc, BakedFont&)`); `BitmapFreeTypeSource` is the one
  registered source this slice, rasterizing the printable-ASCII range with
  FreeType into an R8 `BakedFont` (replaces the old `Font::Load`).
- **`TextLayout` (`layoutText`)** — surface-agnostic: lays out `TextQuad`s in
  local pixel coordinates (pen at origin, baseline y=0), with a
  `MissingGlyphFn` hook (empty this slice = skip, matching old behavior).
  Bytes are still treated as ASCII codepoints; real UTF-8 decoding is later.
- **`TextStyle` / `TextPlacement` / `TextVertex`** — `TextStyle` currently
  honors only `fillColor` (`outlineColor`/`outlineWidth`/`styleIndex` are
  reserved for the effects slice). `TextPlacement` implements `ScreenSpace`
  only (`WorldOriented`/`CameraBillboard`/`AxisBillboard` reserved).
  `TextVertex` is a fixed, tightly packed 32-byte format (`pos`, `uv`, packed
  `fillColor`/`outlineColor`, `styleIndex`) meant to serve bitmap/SDF/MSDF and
  screen/world without another format change.
- **`TextRenderer`** — turns `submit()` calls into `Batch{atlas, backend,
  verts}` merged by `(backend, atlas)`; `Renderer::drawText` just forwards
  into it, and `Renderer::endFrame` flushes each batch via
  `RenderBackend::drawTextBatch` after `compositeBloom`, same draw order as
  v0.17.
- **`RenderBackend::drawTextBatch`** — new backend entry point taking an atlas
  texture id + `TextVertex` batch; the OpenGL implementation uploads to a
  dynamic VBO and draws with a dedicated text shader that unpacks per-vertex
  `fillColor` (`unpackUnorm4x8`) and multiplies it by the R8 atlas coverage —
  per-vertex color instead of v0.17's single per-draw color uniform.

## Migration

`Application`'s debug HUD, `ShooterGame`, and `ArenaGame` all moved to the new
API and the old bitmap `Font` path (`engine/renderer/text/Font.hpp/.cpp`) was
deleted outright — no adapter, no dual path:

```cpp
if (!m_font) {
  engine::FontDesc desc;
  desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf";
  desc.pixelSize = 32;
  m_font = renderer.fonts().load(desc);
}
renderer.drawText(m_font, "Score: " + std::to_string(m_score), placement, style);
```

All three call sites load the same `FontDesc`, so they now share one cached
atlas instead of each baking a duplicate. Output is unchanged from v0.17/v0.27.

## `text_demo_game`

A new demo target (`games/text_demo/`) exercises the system directly: a title
line, colored swatches at different scales, an alpha-pulsing animated line,
and a live tick counter — all through `fonts().load` + `drawText`, with a
footer noting the deliberate slice-1 scope ("bitmap backend; SDF/outline/glow
next").

## Scope for this slice (reserved seams, not implemented)

Per ADR-7 in the design spec, this slice registers only the bitmap backend and
`ScreenSpace` placement. The following are architected for (vertex format,
`FontBackend` enum, `TextPlacement` modes, `styleIndex`) but **not
implemented**, and land in later slices:

- SDF / MSDF backends and offline/runtime atlas generation.
- Effects — outline, cartoon stroke, glow, drop-shadow, gradient — and the
  per-batch style-table UBO/SSBO that would drive them.
- Rich text (style runs, per-character color) and a markup front-end.
- World-space 3D text (`WorldOriented`/`CameraBillboard`/`AxisBillboard`,
  HDR/bloom participation, UI vs world passes).
- Unicode decoding, fallback fonts, kerning/shaping.

## Testing

New unit tests (Catch2, no GL/FreeType): `test_font_desc.cpp` (`FontDesc`
equality/hash for cache-key correctness), `test_font_manager.cpp` (bake-once,
cache-hit on repeated identical `FontDesc`, via a stub source + atlas
factory), `test_text_style.cpp` (`TextVertex` is exactly 32 bytes, `packColor`
RGBA8 packing/clamping, default style/placement values), `test_text_batch.cpp`
(`TextRenderer::submit` emits 6 verts per visible glyph and merges glyphs
sharing an atlas into one batch). `test_text_layout.cpp` carries over from
v0.17 unchanged (local-coordinate quad placement).

- **Behavioral:** HUD/shooter/arena screenshots are pixel-identical to
  v0.17/v0.27 output; full suite green (see below).

## Next Milestones

- Slice 2: `OfflineMsdfSource`, SDF/MSDF shader programs, `pxRange` — crisp
  scaling at any size.
- Slice 3: single-pass effects (outline/cartoon-stroke/glow/shadow/gradient)
  driven by the style-table UBO.
- Slice 4: rich text (style runs, per-character color, markup).
- Slice 5: runtime MSDF (optional `msdfgen`), dynamic atlas, lazy-on-miss glyphs.
- Slice 6: world-space 3D text and remaining placement modes.
