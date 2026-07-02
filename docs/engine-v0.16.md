# Engine v0.16 — Steering Behaviours

v0.16 introduces the Reynolds **force-based** steering model and a seek-the-player
behaviour: bots accelerate toward the player and chase it, instead of drifting at a
constant velocity. Collision (v0.15) and wall bounce (v0.14) are unchanged.

See `docs/engine-v0.15.md` for the collision this builds on.

## Model

Previously a bot's velocity was fixed at spawn (only redirected by collisions). Now
each step a bot computes a steering **force**, accelerates, and clamps to a max
speed:

```
force = seek(botPos, botVel, playerPos, maxSpeed, maxForce);
vel  += force * dt;
vel   = truncate(vel, maxSpeed);
```

The player stays **input-driven** (its velocity is set by WASD, not steering).

## The steering library

`engine/ai/Steering.hpp` (header-only, EnTT-free, planar XZ):

- `truncate(v, maxLen)` — clamp a vector's length (used for both the max-force and
  max-speed caps).
- `seek(pos, vel, target, maxSpeed, maxForce)` — `desired = normalize(target -
  pos) * maxSpeed`; returns `truncate(desired - vel, maxForce)`. Zero at the target.
- `flee(pos, vel, target, maxSpeed, maxForce)` — the same, directly away.

`flee` is shipped and unit-tested but not used in the demo yet (reserved for the
v0.17 combat/predator-prey work).

## Behaviour & sim order

Bots seek the player. `ArenaGame::stepSim`:

1. **Input → player velocity** (unchanged).
2. **Bot steering** — read the `Player` entity's position; for each `Bot`,
   `seek` it, accelerate, clamp to `kBotMaxSpeed`.
3. **Integrate** — `position += velocity * dt`.
4. **Agent collision** (v0.15, unchanged).
5. **Wall bounce** (v0.14, unchanged).

Params: `kBotMaxSpeed = 2.5`, `kBotMaxForce = 8.0`; the player moves at `3.0`, so it
can outrun the swarm — a real chase. The deterministic warm-up has all bots converge
on the stationary (no-input) player, so a headless screenshot shows them clustered
around the centre (held apart by collision).

## Testing

- Unit (Catch2, no GL, no EnTT): `truncate` (short unchanged / long scaled), `seek`
  (toward target from rest, capped at maxForce, zero at target), `flee` (away). 51
  test cases total.
- Behavioral: `arena_game` shows the bots clustered around / chasing the player
  rather than evenly spread; the player pulls away when moving (WASD).

## Next Milestones

- **v0.17 — Health / combat loop** (uses `flee` for fleeing/low-health behaviour).
- **v0.18 — Arena game rules.**
- Later steering: wander, arrive, pursue/evade, flocking (align/cohere/separate),
  obstacle avoidance.
