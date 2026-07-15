# Engine v0.32 — Rich Text (Span List)

v0.32 lets a single `drawText` call carry more than one `TextStyle` — a run
of ordered `TextSpan`s, each with its own color/outline/glow/shadow, laid
out as one continuous string and submitted in one draw. It reuses the v0.30
per-batch style table verbatim; there is no shader/UBO/GPU change. Design
rationale:
`docs/superpowers/specs/2026-07-15-engine-v0.32-rich-text-design.md`.

## What's new (`engine/renderer/text/`)

- **`TextSpan`** (new header `TextSpan.hpp`) — `{ std::string_view text;
  TextStyle style; }`. `text` is not retained past the synchronous submit
  call; the renderer never stores a `TextSpan` across frames.
- **Stateful layout seam** (`TextLayout.hpp`) — `appendTextLayout(glyphs,
  text, TextLayoutState& state, std::vector<TextQuad>& out, onMissing)`
  appends one span's quads starting from `state.penX`/`state.penY` and
  advances `state` in place. `TextLayoutState` holds `penX`, a reserved
  `penY` (multiline not yet supported), and a reserved
  `std::optional<char32_t> previousGlyph` (kerning seam; `Glyph` has no
  kerning data yet, so it's written but unread). `layoutText` is now a thin
  wrapper — construct a local `TextLayoutState`, call `appendTextLayout`
  once, return the buffer — so there is a single layout core instead of a
  duplicated one. Per-character behavior (spaces, missing glyphs, the
  byte-as-codepoint handling) is unchanged.
- **`TextRenderer::submit(font, std::span<const TextSpan> spans, placement,
  w, h)`** is the new core. One `TextLayoutState` and one shared quad buffer
  carry the pen continuously across every span in the run — layout never
  restarts at a span boundary, so shaping/kerning can't drift there. For
  each non-empty span that produces at least one visible glyph, it calls the
  existing `acquire(backend, atlas, pxRange, style)` to dedup the style into
  the batch's table (or start a new batch past `kMaxStylesPerBatch` = 64,
  same cap-split as v0.30) and stamps that `styleIndex` onto the span's
  quads. Per-quad → NDC emission is the same code as the single-style path.
  The existing single-style `submit(font, text, placement, style, w, h)` now
  builds a one-element `TextSpan` and forwards to this core, so the two
  paths cannot drift.
- **`Renderer::drawText(font, std::span<const TextSpan> spans, placement)`**
  — a null-font-guarded overload alongside the existing single-style
  `drawText`; both end up in `TextRenderer`'s batches and go through the
  unchanged `endFrame` → `drawTextBatch` → single-pass SDF shader path.
- **Span rules**, all enforced in `submit` and locked by
  `tests/test_text_batch.cpp`:
  - **Empty span** (`text.empty()`): skipped entirely — no pen movement, no
    style-table entry.
  - **Invisible span** (all spaces, or every codepoint missing with no
    substitute): `appendTextLayout` still advances the pen as layout
    dictates, but since it produced no quads the span claims no style
    entry.
  - **>64 distinct styles in one run**: flows into additional batches via
    `acquire`'s existing cap-split — deterministic, no clamping or silent
    style reuse. Because the layout state lives outside the batch, glyph
    positions stay continuous across the split.
  - **Geometry invariant**: a span boundary changes style only. `"AV"` as
    one span and as `{"A", s1}, {"V", s2}` produce identical glyph vertex
    positions — verified directly (`"AV" split == "AV" whole`) and
    indirectly (single-style `submit` vs. an equivalent one-span rich
    `submit` produce identical geometry and style-table data).
  - Whitespace/missing-glyph/fallback handling matches the pre-existing
    single-style path exactly, since both now run through the same
    `appendTextLayout`.
- **`text_demo`** adds a rich line under the SDF column: `"Rich: "` (plain
  fill), `"red "` (red fill), `"glow "` (fill + outer glow), `"outline"`
  (fill + outline) — four spans, four distinct styles, one `drawText` call.

## Scope for this slice

- **No GPU/shader/UBO changes.** The style-table UBO, binding, and SDF
  fragment shader are exactly as v0.30 left them; rich text is purely a
  layout + submission change that reuses `acquire`/`toGpuStyle`/`packColor`.
- **Reserved, not implemented:** a markup parser (e.g. `[c=red]…[/c]`) to
  produce spans from a single string; gradient interpolation across a run;
  mixed fonts/sizes per span. The `previousGlyph` (kerning) and `penY`
  (multiline) seams in `TextLayoutState` exist for future slices but are
  unused this slice — no kerning is applied and no vertical line breaks
  happen.
- Span coalescing (merging adjacent equal-style spans before layout) isn't
  needed: equal consecutive styles already dedup to one table entry via
  `acquire`.

## Testing

`tests/test_text_batch.cpp` adds nine `"rich: …"` cases on a stub
`FontAsset`, exercising: distinct styles get distinct `styleIndex`
boundaries; equal styles dedup to one table entry; a trailing space in one
span advances the pen into the next; `"AV"` split equals `"AV"` whole;
empty spans move nothing and claim no style; missing glyphs across a span
boundary equal the same codepoints as one string; single-style `submit`
equals an equivalent one-span rich `submit`; a run of >64 distinct styles
flushes into an additional batch with correct per-glyph indices; and layout
state (including a missing-glyph control character) continues identically
across a span boundary. All pre-existing `[textbatch]`/`[text]` tests stay
green — they exercise the refactored `layoutText`/`appendTextLayout`
directly.

## Next Milestones

- Markup parser front-end that emits `TextSpan`s from an inline-tag string.
- Gradient fill across a run, and multi-font/multi-size spans.
- Real UTF-8 decoding, kerning (consuming `previousGlyph`), and multiline
  layout (consuming `penY`) — the seams exist; the behavior is unchanged
  this slice.
- Slice 6 (world-space 3D text) remains as noted in v0.30/v0.31.
