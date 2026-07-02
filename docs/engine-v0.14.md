# Engine v0.14 — Reusable Engine Library & ECS Arena Game

v0.14 pivots from the renderer arc to gameplay. It does two things: extracts the
engine into a reusable static library, and stands up a **second game** on it — an
ECS-driven arena with a fixed-timestep simulation, a WASD player, and ~48 wandering
bots that bounce off the walls. `BotArenaGame` (the renderer showcase) is preserved
unchanged.

## Layout & build

```
engine/                 -> bot_arena_engine (STATIC library)
game/BotArenaGame.*     -> bot_arena     (renderer showcase, unchanged)
games/arena/            -> arena_game    (ECS gameplay)
  ArenaGame.{hpp,cpp}
  Components.hpp
  main.cpp
```

- **`bot_arena_engine`** — all `engine/**` sources as a static library, exposing
  its include dirs, the `BOTARENA_ASSET_DIR` define, and its third-party deps
  (SDL3/glad/glm/EnTT/spdlog/yaml-cpp/imgui/Threads) **PUBLIC**, so any game that
  links it inherits everything.
- **`bot_arena`** = `src/main.cpp` + `game/BotArenaGame.cpp`, links the engine lib.
  Behavior is byte-for-byte unchanged (verified by an identical screenshot before
  and after the refactor).
- **`arena_game`** = `games/arena/*`, links the engine lib — proof the engine is
  reusable.
- `bot_arena_tests` is unchanged plus the two new helper tests.

## Simulation model

The `engine::Layer` gives `onUpdate(dt)` then `onRender`. `ArenaGame` runs a
**fixed-timestep** loop in `onUpdate`:

```
m_accumulator += dt;
FixedStep fs = fixedTimestep(m_accumulator, 1/60, 5);
m_accumulator = fs.remainder;
for (fs.steps)  stepSim(1/60);
```

`stepSim` is two systems over the EnTT registry:
1. **Input → player velocity:** WASD sets the `Player` entity's `Velocity` in the
   arena XZ plane (speed 3).
2. **Movement + wall collision:** `position += velocity * dt`, then
   `resolveWallBounce` clamps each agent to the arena interior (`±4.75` XZ) and
   reflects the crossing velocity component. Bots move at speed 2; every agent stays
   inside the walls.

`onRender` iterates `registry.view<Transform>()` and submits the unit cube with the
player-or-bot material and the entity transform to the deferred renderer, on top of
the arena walls, ground, and four corner point lights.

## Tested core (pure, header-only, EnTT-free)

- `engine/physics/Collision.hpp` — `resolveWallBounce(pos, vel, boundsMin,
  boundsMax, radius) -> {position, velocity}`.
- `engine/core/FixedTimestep.hpp` — `fixedTimestep(accumulated, step, maxSteps) ->
  {steps, remainder}` (caps runaway backlog).

These are the systems' math; the EnTT-iterating systems that call them are
screenshot-verified.

## Screenshot warm-up

Screenshot mode renders a single frame (`dt ≈ 0`), so a headless capture would show
the spawn state. `onAttach` runs a **deterministic 120-step warm-up** after spawning
so the captured frame shows a genuinely simulated arena (bots advanced along their
velocities, many already bounced against the walls; the player, with no input during
warm-up, stays at center). Interactive play is unaffected.

## Testing

- Unit (Catch2, no GL, no EnTT): `resolveWallBounce` (interior untouched; clamp +
  reflect past min/max walls) and `fixedTimestep` (0 steps under one step; correct
  count + remainder; backlog cap). 42 test cases total.
- Behavioral: `bot_arena` screenshot unchanged; `arena_game` screenshot shows the
  player + ~48 bots inside the walled arena, positioned by the simulation.
  Interactive: WASD moves the player; bots wander and bounce.

## Next Milestones

- **Gameplay:** bot-bot collision, AI behaviours (seek/avoid), health/combat,
  camera-follow, a shared arena-scene abstraction between the two games.
- **Renderer (deferred earlier):** `.hdr` environment loading, light culling
  (tiled/clustered).
