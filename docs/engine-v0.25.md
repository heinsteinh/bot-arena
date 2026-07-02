# Engine v0.25 — Top-Down Space Shooter (Core)

v0.25 adds `shooter_game`, a fifth game example: a top-down space shooter's core loop —
fly the player ship, auto-fire projectiles, enemy ships spawn and seek the player, and
hits destroy them for score. It's a capstone that reuses the ECS/fixed-timestep sim,
steering, model loading, and the text HUD. Combat/explosions/waves/asteroids are
follow-up milestones.

See `docs/engine-v0.19.md` (combat) and `docs/engine-v0.22.md` (models) for the pieces
this composes.

## Ship roster

| Role | Model | Tris |
| --- | --- | --- |
| Player | `asteroid-game/Ships/Viper.obj` | 346 |
| Grunt (tier 0) | `SpaceGame/Spaceship.obj` | 540 |
| Heavy (tier 1) | `SpaceGame/Spaceship3.obj` | 1190 |
| Elite (tier 2) | `asteroid-game/Ships/eliteship.obj` | 1803 |
| Bullet | `asteroid-game/Objects/Projectile.obj` | 12 |

The `.obj`/`.mtl` (color materials) are vendored under `assets/`.

## Simulation

`ShooterGame::stepSim` (fixed 60 Hz), EnTT components `Transform{pos,scale,yaw}`,
`Velocity`, `Player`, `Enemy{tier}`, `Bullet{life}`:

1. **Player**: WASD moves (direct velocity, faces heading). When idle it holds position
   and **auto-aims at the nearest enemy**.
2. **Auto-fire**: a `Bullet` spawns at the ship nose along its heading on a cooldown,
   with a short life.
3. **Enemies**: each `seek`s the player (v0.16), clamped to its tier speed, and faces
   its heading; a timer spawns weighted tiers at the arena edge up to a cap.
4. **Integrate + hits**: everything moves; bullets age and cull out of bounds; a
   bullet overlapping an enemy (`circlesOverlapXZ`) destroys both and adds the tier
   weight to `score`; an enemy reaching the player breaches (removed, `leaks++`).

## The tested core

`engine/gameplay/ShipControls.hpp` (pure): `headingToYaw(v)` = `atan2(v.x, v.z)`,
`forwardFromYaw(yaw)` = `{sin, 0, cos}` (its XZ inverse), and `circlesOverlapXZ` for
hits. Unit-tested; the ECS/render loop is screenshot-verified.

## Rendering

The five ship models load on the first frame (v0.22/24 loader, per-submesh materials);
each entity renders as `translate * rotateY(yaw + shipOffset) * scale *
fitToUnitTransform(bounds)` on a dark ground under a top-down orbit camera. A text HUD
(v0.17) shows `Score` / `Enemies` / `Leaks`. (`shipOffset` corrects the models' base
orientation.)

## Testing

- Unit (Catch2, pure): `headingToYaw`/`forwardFromYaw`/`circlesOverlapXZ`. 77 test
  cases total.
- Behavioral: the `shooter_game` screenshot shows the Viper firing at converging enemy
  ships with a non-zero `Score`. Other games unchanged.

## Next Milestones

- Player health/lives + game-over; enemy fire back.
- Explosions (particles v0.20) on kills; waves + win/lose.
- Asteroids as destructible obstacles; power-ups; mouse aim.
