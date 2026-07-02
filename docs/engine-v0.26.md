# Engine v0.26 — Shooter Player Health & Enemy Fire

v0.26 turns `shooter_game` into a real firefight. The player has health and 3 lives;
enemies face the player and shoot back; bullets have factions (green player / red enemy)
and point along their flight. Death respawns; running out of lives resets the run.
Reuses the tested `adjustHealth` (v0.19) and ship-heading helpers (v0.25).

See `docs/engine-v0.25.md` for the shooter core this builds on.

## What changed

- **Components:** the player gains `Health{current, max}` (100); `Bullet` gains a
  `fromPlayer` faction; `Enemy` gains a `fireTimer`. Enemies also carry a `Health`
  component so tougher tiers survive more than one hit.
- **Enemies face the player** (`yaw = headingToYaw(playerPos - enemyPos)`) and, when
  within `kFireRange`, fire a slower (dodgeable) bullet at the player on a staggered
  cooldown.
- **Bullet factions:** player bullets (**green**) damage enemies; enemy bullets
  (**red**) damage the player. Both render with a per-faction override material and are
  rotated along their velocity so the nose leads the flight.
- **Enemy health:** a player bullet removes one hit point; grunts die in 1, heavies in
  3, elites in 5. This keeps enemies alive long enough to shoot back (a one-shot enemy
  never fires).
- **Damage & lives:** an enemy bullet deals `-10`, a ram (overlap) deals `-25` and
  destroys the enemy — both via `adjustHealth`. HP ≤ 0 loses a life and respawns the
  player at centre with full HP (clearing enemy bullets); 0 lives resets the run (clear
  enemies + bullets, `score = 0`, `lives = 3`, HP full).
- **HUD:** `Score` / `HP: cur/max` / `Lives` / `Enemies`.

## Note on scope

The v0.26 plan destroyed enemies in one hit. During execution that proved to undercut
the whole feature — the player's auto-aim one-shots every enemy before it can fire, so
no enemy bullets appear. Enemy `Health` (reusing the same component + `adjustHealth`)
was added so the firefight is genuinely two-way.

## Testing

Combat wiring on already-tested helpers (`adjustHealth` v0.19;
`headingToYaw`/`forwardFromYaw`/`circlesOverlapXZ` v0.25) — verified by screenshot.

- **Behavioral:** the `shooter_game` screenshot shows the player firing green bullets
  while an enemy (nosed at the player) fires red bullets back, with `HP` below max and
  `Lives`/`Score` in the HUD. 77 unit test cases still pass; other games unchanged.

## Next Milestones

- Explosions on kills (reuse the v0.20 particle system).
- Waves + win/lose progression; power-ups; shields.
- Per-tier enemy fire patterns; mouse aim.
