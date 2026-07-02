# Engine v0.20 — Particle System & Demo Game

v0.20 adds a CPU particle system and renders it as **additive camera-facing billboards
into the HDR scene before bloom**, so particles glow. A new `particles_game` executable
demonstrates three effect types — a burst on every wall collision, a fountain, and a
rising haze column — on the shared engine library, leaving `bot_arena` and `arena_game`
untouched.

See `docs/engine-v0.16.md` for the gameplay-helper pattern this follows.

## Simulation (`engine/particles/`)

- `Particle { position, velocity, color, gravity, size, life, maxLife }`.
- Pure helpers (unit-tested): `integrateParticle(p, dt, gravity)` (velocity += gravity·dt;
  position += velocity·dt; life -= dt), `lifeFraction` (`clamp(life/maxLife)`),
  `renderColor` (`color · lifeFraction`, fades to black), `isDead`.
- `ParticleSystem`: a pooled `std::vector<Particle>` with `emit(EmitParams, origin,
  rng)` and `update(dt)` (integrate each by its own gravity, then swap-remove dead).
- `EmitParams` is a preset: count, speed range, base direction + spread, color, size
  range, life range, gravity. Three presets ⇒ three effects; "and more" is just more
  `EmitParams`.

## Renderer path

- `ParticleInstance { vec3 position; float size; vec4 color }` — per-particle GPU data.
- Backend `drawParticles(instances, count)` — an instanced quad billboard (right/up
  from `u_view`, per-instance center/size/color), **additive (`GL_ONE, GL_ONE`), depth
  off**, with a round `smoothstep` falloff. Drawn into the HDR scene right after the
  emissive light billboards (before bloom), so particles bloom.
- `Renderer::submitParticles(...)` appends per frame (cleared in `beginFrame`);
  `endFrame` flushes them.

## Demo (`games/particles/`, third executable)

`particles_game` = `main.cpp` + `ParticlesGame.cpp`, linking `bot_arena_engine`. Cubes
bounce inside a box (fixed-timestep); each wall bounce `emit`s a warm radial **burst**,
a fixed emitter runs a cyan **fountain**, and another runs a rising **haze** column.
`onRender` submits every live particle as a `ParticleInstance{position, size,
vec4(renderColor(p), 1)}`. Orbit camera; a deterministic warm-up so the capture shows
particles in flight.

## Additive-blend note

The billboard pass is purely additive (built for glowing effects), so overlapping
particles brighten toward white — great for the fountain and bursts, but a grey smoke
plume reads as a soft glowing haze rather than opaque smoke. True (occluding) smoke
needs an alpha-blended pass; that is future work.

## Testing

- Unit (Catch2, no GL): `integrateParticle`/`lifeFraction`/`renderColor`/`isDead`;
  `ParticleSystem` (`emit` adds count within the life range; `update` removes the dead
  and moves the living). 66 test cases total.
- Behavioral: `particles_game` screenshot shows the glowing cyan fountain, warm
  collision bursts, and the haze column; `bot_arena`/`arena_game` unchanged.

## Next Milestones

- Later particles: an alpha-blended pass (real smoke), textured/animated sprites,
  soft-particle depth fade, GPU/compute simulation, sub-emitters/trails.
- Gameplay: arena game rules (score/waves/win-lose).
