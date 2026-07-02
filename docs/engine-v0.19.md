# Engine v0.19 — Health & Combat Loop

v0.19 adds contact-based combat to the arena: bots and the player have Health, deal
damage on contact (player stronger, swarm lethal), and die/respawn — and wounded bots
**flee** to heal and re-engage, turning the v0.16 `flee` primitive into real
behaviour. Renderer and `bot_arena` are untouched.

See `docs/engine-v0.16.md` for the steering this builds on.

## The loop

Each fixed step, after movement/collision:

1. **Contact damage** — for every bot within reach of the player, the bot damages the
   player (`8` DPS) and the player damages the bot (`40` DPS). Bots *not* in contact
   regen (`6`/s). So the player wins any 1-v-1, but a ring of bots is deadly.
2. **Death / respawn** — a bot at `0` HP recycles (full HP + teleport to a random
   arena-edge point) and increments `kills`; the player at `0` HP respawns at center
   with full HP and increments `deaths`.
3. **Flee when hurt** — a bot at/under `35%` HP steers with `flee` instead of `seek`:
   it retreats, heals via regen, then returns. A chase → wound → retreat → re-engage
   loop emerges.

Health: player `100`, bots `30`.

## `stepSim` order

input → **steering by health (`shouldFlee ? flee : seek`)** → integrate → agent
collision → wall bounce → **combat (contact damage / regen)** → **death/respawn**.
Steps 3–5 are unchanged from v0.15/v0.16.

## The tested core

`engine/gameplay/Combat.hpp` (header-only, EnTT/GL-free), beside `Steering.hpp` /
`Collision.hpp`:

- `adjustHealth(current, delta, max)` → `clamp(current + delta, 0, max)` (damage when
  `delta < 0`, regen when `delta > 0`).
- `shouldFlee(current, max, fleeFraction)` → alive and at/under the fraction.

## Contact reach (tuning note)

Collision separates agents to exactly `rA + rB`, which is also the naive contact
distance — so touching bots sit right at the boundary and barely register hits. Combat
therefore uses `dist < botScale + playerScale + kContactMargin` (`0.2`), so the ring of
bots held apart by collision still lands damage. The deterministic warm-up was also
extended to `300` steps (5 s) so combat resolves before the first rendered frame.

## On-screen readout

`ArenaGame` loads its own font and draws `HP: <cur> / <max>` and `Kills: <n>`
bottom-left (clear of the F1 debug HUD top-left), reusing the v0.17 text renderer.

## Testing

- Unit (Catch2, no GL/EnTT): `adjustHealth` (damage / heal / clamp both ends) and
  `shouldFlee` (full → false, at/under fraction → true, dead → false). 61 test cases
  total.
- Behavioral: `arena_game` logs `HP` and `kills` after the warm-up (e.g. `HP≈55,
  kills=21, deaths=1`), and the screenshot shows the swarm on the player with the
  `HP`/`Kills` readout — combat resolved end-to-end.

## Next Milestones

- **Arena game rules** — score, waves, and win/lose conditions (the combat counters
  feed directly into this).
- Later combat: attack input/abilities, weapons/projectiles, damage cooldowns,
  per-entity health bars, factions.
