# Engine v0.21 — ImGui Integration & Particle Editor

v0.21 wires up Dear ImGui the Hazel way — as an `ImGuiLayer` driven through the layer
stack, with a `Layer::onImGuiRender` hook every game can override — and demonstrates it
with a live particle editor in `particles_game`: pick an effect type and tweak its
`EmitParams` with sliders and a color picker, live. `bot_arena`/`arena_game` are
untouched (they simply don't override `onImGuiRender`).

See `docs/engine-v0.20.md` for the particle system this drives.

## Hazel-style integration

- **`Layer::onImGuiRender()`** — a no-op virtual every layer can override to emit ImGui
  widgets.
- **`engine::ImGuiLayer`** (a `Layer`, `engine/imgui/`): `onAttach` creates the ImGui
  context and initializes the SDL3 + OpenGL3 backends; `onDetach` shuts them down;
  `begin()`/`end()` bracket the per-frame ImGui pass (`NewFrame` … `Render` +
  `RenderDrawData`).
- **`Application`** owns the `ImGuiLayer` (built from `Window::nativeHandle()` +
  `GraphicsContext::nativeContext()`). Each frame, after `renderer.endFrame()` (scene on
  the default framebuffer): `imgui.begin()` → every layer's `onImGuiRender()` →
  `imgui.end()`, then screenshot/swap — so panels draw on top and appear in
  screenshots.
- **Events:** `Window::setEventCallback` — `SdlWindow` forwards each `SDL_Event` to the
  callback; the app forwards it to `ImGui_ImplSDL3_ProcessEvent` and, when ImGui wants
  the mouse/keyboard, returns `true` so `SdlWindow` **skips feeding engine `Input`**
  (dragging a slider no longer spins the camera). `SdlWindow` stays ImGui-agnostic.
- **Context:** `GraphicsContext::nativeContext()` exposes the `SDL_GLContext` for init.

Every game gets an ImGui overlay for free via `onImGuiRender`.

## The particle editor (`particles_game`)

`ParticlesGame::onImGuiRender` builds a "Particle Editor" panel:
- A **Type** combo of named presets — **Burst, Fountain, Smoke** (v0.20) plus three new
  ones: **Jet** (tight fast upward), **Nova** (big radial gold explosion), **Drizzle**
  (arcs up, rains down). Selecting one loads its preset into an editable `EmitParams`.
- **Sliders + a color picker** for count, speed min/max, spread, color, size min/max,
  life min/max, and gravity, plus a live particle count.
- A central **live emitter** (`m_editor`) continuously emits with the edited params, so
  tweaks are visible instantly. The ambient bouncers (collision bursts), fountain, and
  smoke from v0.20 remain.

## Testing

This is a UI/integration milestone, verified by screenshots (no headless ImGui unit
test); the particle sim's v0.20 unit tests are unchanged (66 test cases).

- Behavioral: `particles_game` screenshot shows the "Particle Editor" panel (combo +
  sliders + color + live count) over the glowing editor emitter; `bot_arena` and
  `arena_game` render unchanged.

## Next Milestones

- ImGui: docking/viewports, save/load presets to disk, an entity inspector, per-game
  panels (e.g. an arena tuning panel).
- Particles: an alpha-blended pass (real smoke), textured sprites, GPU simulation.
- Gameplay: arena game rules (score/waves/win-lose).
