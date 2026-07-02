# Engine v0.18 — Engine Debug HUD

v0.18 turns the v0.17 FPS line into an **F1-toggled engine debug HUD** — frame timing,
camera pose, renderer counts, and JobSystem load — with the JobSystem and Renderer
instrumentation to feed it. App-level, so both games show it with no game-code changes.

See `docs/engine-v0.17.md` for the text renderer this builds on.

## The HUD

Nine lines, top-left (toggle with **F1**, starts visible):

```
FPS: 60 (16.6 ms)
Res: 1280x720
Cam: (8.0, 7.0, 8.0)          # world position
Fwd: (-0.6, -0.5, -0.6)       # view forward
Draws: 2048                    # merged draw submissions
Lights: 16                     # active point lights
Lanes: 16                      # renderer/job worker lanes
Jobs: 1 disp / 16 batch / 2048 items
LaneBatches: 2 1 1 1 1 1 1 2 1 1 1 1 0 1 0
```

The last two lines are the "how well is the job system doing" view: a near-even
`LaneBatches` spread means work is balanced across threads. The numbers are real per
game — `bot_arena` shows `Lights: 16` and a busy `Jobs` line (its 2048-cube swarm is
generated with `parallelFor`); `arena_game` shows `Lights: 4` and `Jobs: 0` (it
submits meshes directly, no parallel generation).

## Instrumentation

- **`JobSystem::Stats`** (`stats()` / `resetStats()`) — per-frame `dispatches`,
  `batches`, `items`, and `laneBatches[lane]`. No atomics: the main thread writes the
  totals in `parallelFor`; each lane increments only its own `laneBatches` slot.
  `Application` calls `resetStats()` at frame start (workers idle between dispatches).
- **`Renderer::RenderStats`** (`stats()`) — `drawCount = m_merged.size()`,
  `pointLights`, `laneCount`, `cameraPos`/`cameraFwd` derived from the frame's
  `CameraUniforms`. The draw count is read before `endFrame` merges, so it is
  1-frame lagged (standard for a HUD; it reads `0` in a single-frame headless
  capture).
- **`Key::F1`** added to the `Key` enum and mapped from `SDLK_F1`.

## The tested core

`formatDebugLines(const DebugStats&)` (`engine/core/DebugOverlay.hpp`) is a pure,
GL-free function turning an aggregated `DebugStats` into the HUD's text lines (floats
via `snprintf("%.1f")`). `Application` fills `DebugStats` from `Time`,
`Renderer::stats()`, and `JobSystem::stats()`, then draws each line via the v0.17
`Renderer::drawText`.

## Testing

- Unit (Catch2, no GL): `formatDebugLines` (exact expected lines + line count, empty
  `laneBatches`); extended `test_job_system` (after `parallelFor(100, 10)`:
  `dispatches == 1`, `items == 100`, `batches == 10`, `sum(laneBatches) == 10`). 59
  test cases total.
- Behavioral: `bot_arena` and `arena_game` screenshots show the multi-line HUD with
  real per-game engine data.

## Next Milestones

- **Gameplay resumes — health / combat loop** (uses `flee`), then arena game rules.
- Later HUD: GPU/pass timings, memory (arena/pool bytes), plots/history, and the
  game-specific camera-controller mode (needs a per-game hook).
