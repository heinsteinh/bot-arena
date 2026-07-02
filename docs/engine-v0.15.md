# Engine v0.15 — Bot-Bot Collision & Separation

v0.15 makes arena agents solid. Each fixed step, an O(n²) pass resolves circular
(XZ) overlaps between agents — separating them and, when they are approaching,
bouncing them — so no two agents interpenetrate. Wall bounce is unchanged.

See `docs/engine-v0.14.md` for the ECS arena this extends.

## Why it matters (the visible fix)

v0.14 had no agent-agent collision, so the ~48 randomly-spawned bots overlapped and
their cubes intersected at angles — producing jumbled, hollow-looking "open corner"
shapes (the mesh was fine; the cubes were simply interpenetrating). v0.15 pushes
overlapping agents apart, so each renders as a clean, solid, separated cube.

## The pure helper

`engine/physics/Collision.hpp` — `resolveAgentPair`, beside `resolveWallBounce`:

```cpp
struct AgentPair { glm::vec3 posA, velA, posB, velB; };
AgentPair resolveAgentPair(posA, velA, rA, posB, velB, rB);
```

Circle-vs-circle in the XZ plane (radius = the agent's `Transform.scale`; `y`
untouched):

- **Separation:** if the XZ distance < `rA + rB`, push each agent half the
  penetration along the horizontal normal.
- **Velocity:** if the pair is *approaching* (`vaN - vbN > 0`), swap the normal
  component of their velocities (equal-mass elastic bounce). If already separating,
  velocities are left alone (no jitter).
- **Degenerate:** coincident centers push apart along `+X` (no NaN).

## System integration

`ArenaGame::stepSim` runs three passes over `view<Transform, Velocity>`:

1. **Integrate** — `position += velocity * dt` for all.
2. **Agent collision (O(n²))** — collect the entities into a `std::vector`,
   double-loop `i < j`, apply `resolveAgentPair`, write back. ~49 agents ≈ 1,176
   pairs/step — trivial. **No spatial grid** (deferred until agent counts grow).
3. **Wall bounce (unchanged)** — `resolveWallBounce` each agent last, so any
   separation that nudged an agent toward a wall is still clamped inside.

The player is included as a solid agent. The input→player-velocity step is
unchanged. The v0.14 deterministic warm-up now also resolves collisions, so the
headless screenshot shows cleanly separated agents.

## Testing

- Unit (Catch2, no GL, no EnTT): `resolveAgentPair` — separated pair untouched;
  approaching pair separated + normal velocities swapped; receding pair separated
  but velocities kept; coincident pair pushed apart without NaN. (`resolveWallBounce`
  / `fixedTimestep` tests stay.) 43 test cases total.
- Behavioral: `arena_game` shows agents spaced out with no interpenetrating cubes;
  the player and walls are unchanged.

## Next Milestones

- **v0.16 — Steering behaviours** (seek / flee / wander forces on bot velocity).
- **v0.17 — Health / combat loop.**
- **v0.18 — Arena game rules.**
- Later: a spatial grid / broadphase when agent counts grow past hundreds.
