# Engine v0.27 — Shooter Explosions on Kills

v0.27 spawns particle explosions when ships die in `shooter_game` — a tier-scaled burst
at each enemy kill (bullet or ram) and a bigger blast when the player dies. It reuses the
v0.20 particle system verbatim; there is no new engine code.

See `docs/engine-v0.26.md` for the combat this builds on and `docs/engine-v0.20.md` for
the particle system.

## What changed

- **`ShooterGame` gains** one `engine::ParticleSystem m_explosions;` (reusing the
  existing `m_rng`).
- **`enemyExplosion(tier)`** (local `EmitParams` preset): a radial warm-orange burst
  whose count / speed / size scale with the enemy tier (grunt small pop → elite big
  blast). Emitted at the enemy position when its health is spent *or* when it rams the
  player.
- **`playerExplosion()`**: a bigger, brighter gold-white blast emitted at the player's
  position on death (before respawn).
- **Update + render:** `m_explosions.update(dt)` runs at the end of `stepSim`; in
  `onRender` the live particles become `engine::ParticleInstance`s
  (`{position, size, vec4(renderColor(p), 1)}`) and are drawn with
  `renderer.submitParticles(...)` — additive camera-facing billboards in the HDR scene,
  so they glow through bloom.

The presets are local free functions (anonymous namespace) because the particles_game
presets are not exported from the engine.

## Testing

Integration on the already-tested `ParticleSystem::emit`/`update` (v0.20) — verified by
screenshot.

- **Behavioral:** the `shooter_game` screenshot shows glowing orange bursts at enemy
  kill sites (with fading embers from earlier kills). A forced-death run confirmed the
  player blast fires on death (Lives drops, the player respawns, and the gold burst
  appears at the death site). 77 unit test cases still pass; other games unchanged.

## Next Milestones

- Muzzle flashes on fire and per-bullet impact sparks.
- Engine / thruster trails behind the ships.
- Screen shake on the player's death; waves + win/lose progression.
