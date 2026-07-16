# Engine v0.33 — World Billboard Text (Damage Numbers)

v0.33 adds `CameraBillboard` placement: a text run anchored at a world point,
transformed local → world → clip on the GPU via the camera, always facing
the camera, and drawn always-on-top of the scene. The SDF **fragment**
shader is reused verbatim from screen text — only a new **vertex** stage
projects the glyph quads through the camera's world-space right/up basis.
Use case: floating damage numbers, shown by a new `billboard_demo` game.
Design rationale:
`docs/superpowers/specs/2026-07-15-engine-v0.33-billboard-text-design.md`.

## What's new (`engine/renderer/text/`, `engine/renderer/opengl/`)

- **`TextPlacement::cameraBillboard(worldPos, worldUnitsPerPixel)`**
  (`TextPlacement.hpp`) — a named factory alongside the existing `screen()`
  one. It sets `mode = CameraBillboard`, `worldPos`, and clamps
  `worldUnitsPerPixel` into `scale` to a small positive minimum
  (`> 1e-5f`), so a zero or negative value can never mirror the text.
- **World-space sizing.** `worldOffset = localPixelOffset * worldUnitsPerPixel`
  — a true physical size that shrinks with distance under perspective, not
  a constant-screen-size label.
- **`WorldTextVertex`** (new header, `WorldTextVertex.hpp`) — `{ vec3
  anchor; vec2 offset; vec2 uv; uint32_t styleIndex; }`, 32 bytes. SDF-only,
  no per-vertex color: the SDF effect fragment reads fill/outline/glow/
  shadow from the per-batch style table by `styleIndex`, exactly as screen
  SDF text does.
- **World vertex shader** (`OpenGLBackend.cpp`, `m_worldTextProgram`) reads
  the camera UBO (binding 0) and derives the camera-facing basis the same
  way the engine's particle/emissive billboard shaders already do:
  ```glsl
  mat3 camRot = transpose(mat3(uView));
  vec3 world = aAnchor + camRot[0] * aOffset.x + camRot[1] * aOffset.y;
  gl_Position = uViewProjection * vec4(world, 1.0);
  ```
  It is paired with the **existing** SDF effect fragment shader, now
  hoisted to a shared `kSdfEffectFs` constant and reused unchanged by both
  the screen and world programs — no fragment-shader duplication, and the
  varying interface (`vUV`, `flat vStyleIndex`) matches exactly.
- **`FontAsset::supportsDistanceFieldEffects()`** — `true` for
  `FontBackend::SDF`/`MSDF`. Billboard submission checks this capability
  (not a concrete-class check); a non-DF font logs `spdlog::warn` and
  produces no batch — a documented no-op, not an error.
- **`TextRenderer` parallel world batch list.** `WorldBatch` mirrors the
  screen `Batch` shape (`atlas`, `backend`, `pxRange`, `styles`,
  `std::vector<WorldTextVertex> verts`), exposed via `worldBatches()`. The
  style dedup + 64-style cap-split logic is factored into a shared
  templated `acquireIn(batches, index, backend, atlas, pxRange, style)`
  that both `Batch` and `WorldBatch` use, so the canonical batch-key
  structure (`backend` + `atlas`) is identical on both paths.
- **`submit` routes by `placement.mode`**: `ScreenSpace` goes through the
  unchanged screen path; `CameraBillboard` goes through `submitBillboard`,
  a two-pass world path:
  1. Lay out the **entire ordered span run** with one continuous
     `TextLayoutState`, accumulating quads and, per span, its acquired
     `styleIndex` — no per-span re-layout.
  2. Compute the logical width **once** from the final pen position and
     derive a single centering offset for the whole run.
  3. Emit each glyph's quad as `WorldTextVertex`s using that span's style
     index; every vertex of a run shares the same `anchor` (`worldPos`) —
     only `offset` varies per glyph corner.
  Empty text produces no batch; `>64` distinct styles in one run split into
  another `WorldBatch` on the same atlas, preserving glyph order.
- **Anchor & alignment (`CameraBillboard`, this slice):** horizontal
  center on the run's logical width (`state.penX`, not per-glyph or
  per-span bounds); vertical anchor is the baseline at `worldPos`; local
  layout is y-down, converted to world `+y` up via
  `offset = ((localX - logicalWidth/2), -localY) * worldUnitsPerPixel`;
  single line only.
- **Backend `drawWorldTextBatch(atlasTextureId, verts, pxRange, styles)`**
  (`RenderBackend`, implemented in `OpenGLBackend`) owns a dedicated world
  VAO/VBO/program and explicit render state, set and restored around the
  draw: depth test off, depth write off (`glDepthMask(GL_FALSE)`), face
  culling off (billboard winding is orientation-dependent), straight-alpha
  blending (`GL_SRC_ALPHA`/`GL_ONE_MINUS_SRC_ALPHA`, matching the SDF
  fragment's straight-alpha output). All four are restored after the draw
  so nothing leaks into later passes.
- **Always-on-top overlay ordering.** `Renderer::endFrame` draws, in order:
  scene composite → world billboard batches (`drawWorldTextBatch` per
  `worldBatches()`) → screen-space text batches (`drawTextBatch` per
  `batches()`) — so the HUD always draws over billboards, and both draw
  over the composited scene. `TextRenderer::clear()` clears both lists.
- **`billboard_demo`** (`billboarddemo` namespace, `billboard_demo_game`
  target) shows floating damage numbers over a ring of enemy cubes, orbited
  by a camera with non-zero yaw and pitch. `BOTARENA_ORBIT=1|2` selects an
  alternate orbit angle for rotation-invariance screenshots. Damage numbers
  (`DamageNumber { worldPos, text, color, scaleMul, age }`) are pre-seeded
  in `onAttach` — five labels at different enemies/ages, one crit — so a
  single deterministic frame (`BOTARENA_SCREENSHOT` freezes motion/spawns)
  shows several distances, a bigger crit label, and a partially-faded one
  without full overlap. Each is drawn with
  `renderer.drawText(m_font, d.text, TextPlacement::cameraBillboard(d.worldPos,
  kWorldUnitsPerPixel * d.scaleMul), style)`, outlined for readability and
  colored by magnitude (white → orange → yellow crit). A screen-space HUD
  title is drawn alongside to prove overlay ordering.

## Lifetime contract

Submission is synchronous: `submit` lays out and emits vertices within the
`drawText` call, and neither `TextSpan::text` nor the string argument is
retained past it — a temporary `std::string` is safe to pass directly. The
demo still stores `DamageNumber::text` (formatted once at spawn) since it
draws the same label across multiple frames; that's a demo convenience, not
a renderer requirement.

## Scope for this slice

- **Mode:** `CameraBillboard` only.
- **Reserved, not implemented:** `WorldOriented` (arbitrary transform) and
  `AxisBillboard` placement modes; depth occlusion (world text currently
  always draws on top, ignoring the depth buffer); constant-apparent-size
  scaling (current sizing is true world-space and shrinks with distance);
  world-scaled glow/shadow — the SDF effect's `fwidth`-based glow/shadow
  are screen-derivative-driven (screen-pixel offsets), same as screen text,
  not physically world-scaled; HDR/bloom participation (billboards draw
  post-composite, outside the HDR/bloom pipeline); multiline text (`\n` is
  still a missing glyph, as in screen text).
- Rich-text spans are supported on the billboard path with identical
  geometry semantics to screen rich text (v0.32): a span boundary changes
  style only, never glyph positions.

## Testing

`tests/test_text_batch.cpp` adds 15 CPU-only `"bb: …"` cases (stub SDF and
non-SDF `FontAsset`s, no GL) covering: `CameraBillboard` routes only to
world batches and `ScreenSpace` only to screen batches; every vertex of one
billboard shares the same anchor; horizontal centering uses the whole run's
logical width; y-down layout converts to `+y` up; one-span and multi-span
runs produce identical positions; positive `worldUnitsPerPixel` yields the
expected offsets; non-positive scale is clamped rather than mirrored;
empty text and empty spans allocate no batch/style; equal styles dedup;
>64 styles split world batches without reordering glyphs; different atlases
(pxRange) never merge; a non-DF font produces no batch; and submission from
a temporary string is safe. All pre-existing `[text]`/`[textbatch]` tests
stay green.

Two render/screenshot checks (visually inspected, not asserted in the unit
suite): a rotated + elevated camera (`BOTARENA_ORBIT=1|2`) confirms
billboards stay upright and face the camera at multiple angles without
mirroring/rolling/skewing; and world billboards render underneath the
screen-space HUD title, confirming overlay ordering.

## Next Milestones

- `WorldOriented` (arbitrary transform, not camera-facing) and
  `AxisBillboard` (constrained to one rotation axis) placement modes.
- Depth-occluded world text, drawn inside the scene pass against the depth
  buffer instead of always-on-top.
- A constant-apparent-size option, and world-scaled glow/shadow.
- A shared `TextLayoutResult` (logical + visual bounds) used by both the
  screen and billboard paths; multiline blocks.
