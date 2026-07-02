# Engine v0.17 — FreeType Text Rendering & FPS Overlay

v0.17 adds the engine's first 2D text path — a Hazel-style FreeType font atlas, a
general `Texture2D`, a pure text-layout helper, and a screen-space text-quad renderer
— and uses it for an always-on on-screen **FPS counter** shown by both games. A
renderer/tooling milestone; the deferred 3D pipeline is untouched.

## Pipeline

```
DejaVuSans.ttf --FreeType--> R8 glyph atlas (Texture2D) + GlyphMap (metrics)
                 layoutText(glyphs, text, x, y, scale) --> [TextQuad] (pure)
                 backend.drawText(atlas, quads) --> blended screen-space draw
                 (after compositeBloom, on the default framebuffer)
```

## Components

- **`Texture2D`** (`engine/renderer/Texture2D.hpp`, `opengl/OpenGLTexture2D.*`) — the
  general 2D texture the engine lacked (only `TextureCube` existed). DSA `GL_R8` with
  CPU upload (`setData`), mirroring `TextureCube`'s `Create(...)` factory.
- **`Font`** (`engine/renderer/text/Font.*`) — `Font::Load(ttf, pixelSize)` uses
  FreeType to rasterize printable ASCII (32–126) at 32 px, shelf-packs the glyphs into
  one R8 atlas (1 px padding), uploads it to a `Texture2D`, and records a `GlyphMap`
  (`size`, `bearing`, `advance`, atlas `uvMin/uvMax` per glyph).
- **`layoutText`** (`engine/renderer/text/TextLayout.hpp`) — pure, EnTT/GL/FreeType-
  free. Turns a string + glyph metrics + baseline pen into positioned/UV'd
  `TextQuad`s. The TDD anchor.
- **Backend text draw** (`OpenGLBackend::drawText`) — an inline text shader (samples
  the atlas `.r` as coverage × a color uniform) + a dynamic quad VBO; converts pixel
  corners to NDC, alpha-blends with depth off, restores state.
- **`Renderer::drawText`** — lays out the string and enqueues a `{atlas, quads,
  color}` batch (cleared each `beginFrame`); `endFrame` flushes the batches after
  `compositeBloom`, so text lands on the default framebuffer and in screenshots.

## Layout convention

Pixel space, y-down, origin top-left. `(x, y)` is the baseline pen. Per glyph at
scale `s`: `x0 = x + bearing.x*s`, `y0 = y - bearing.y*s`, `x1 = x0 + size.x*s`,
`y1 = y0 + size.y*s`, `pen.x += advance*s`. Zero-size glyphs (space) advance without a
quad; characters ≥128 or absent from the map are skipped.

## FPS overlay (app-level)

`Application` loads `assets/fonts/DejaVuSans.ttf` once, tracks a smoothed FPS
(`fps = 1/dt`, EMA factor 0.1), and each frame — after the layers render, before
`endFrame` — calls `renderer.drawText(font, "FPS: N", 8, 24, 1, white)`. Both
`bot_arena` and `arena_game` get the overlay with **no game-code changes**
(`BotArenaGame` and `ArenaGame` are untouched).

## Testing

- Unit (Catch2, no GL/FreeType): `layoutText` — single glyph placed by bearing with
  UVs copied; pen advances between glyphs; positions/sizes scale; a space advances but
  emits no quad; empty and unmapped input produce no quads. 56 test cases total.
- Behavioral: `bot_arena` and `arena_game` screenshots each show a crisp `FPS: N`
  top-left. (In a one-frame headless capture `dt ≈ 0`, so the value reads `0`; the
  live value is meaningful interactively — the capture proves the atlas → layout →
  blended-draw path.)

## Asset

`assets/fonts/DejaVuSans.ttf` is vendored (DejaVu is freely redistributable), so the
build is self-contained.

## Next Milestones

- **Gameplay resumes — health / combat loop** (uses `flee`), then arena game rules.
- Later text: multiple simultaneous fonts/sizes, SDF/scalable glyphs, kerning, UTF-8,
  a general 2D sprite/quad renderer, and text alignment/wrapping.
