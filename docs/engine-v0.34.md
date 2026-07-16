# Engine v0.34 — Tangent-Space Normal Mapping

v0.34 adds tangent-space normal mapping to the deferred g-buffer. A per-vertex
`vec3` tangent flows to the mesh geometry pass, which builds a TBN basis and, when
a material carries a normal map, replaces the interpolated vertex normal in
`gNormal` with the sampled, tangent-space-transformed normal. Because the g-buffer
normal is the single source every later pass already reads, PBR lighting, PCF
shadows, SSAO, and IBL all pick up the surface relief for free — no lighting-pass
change. A new `normalmap_demo` game shows a brick wall with vs without the normal
map under a moving light. Design rationale:
`docs/superpowers/specs/2026-07-16-engine-v0.34-normal-mapping-design.md`.

## What's new

- **`a_tangent` vertex attribute (`vec3`, location 3).** The mesh vertex layout
  becomes `position(3) / normal(3) / uv(2) / tangent(3)` — 11 floats/vertex. The
  bitangent is *not* stored; it is derived in-shader as `cross(N, T)`. Two
  producers generate tangents:
  - **Built-in unit cube** (`Renderer::initBuiltins`) — each face gets a constant
    tangent equal to its +U (texture-right) direction, appended per vertex. Face
    order `+X, -X, +Y, -Y, +Z, -Z` → tangents `{0,0,1}, {0,0,-1}, {1,0,0},
    {1,0,0}, {1,0,0}, {-1,0,0}`.
  - **Loaded models** (`ModelLoader`) — `aiProcess_CalcTangentSpace` is added to
    the assimp import flags and `mesh->mTangents[i]` is interleaved into the
    vertex buffer. Meshes without tangents (no UVs) fall back to a `{1,0,0}`
    placeholder, which the shader's `u_hasNormal == 0` path ignores.
- **`Material.normalMap`** (`ResourceRegistry.hpp`) — an optional
  `Ref<Texture2D>`, default `nullptr`. Because `Material` is an aggregate and the
  new field is last, every existing `registerMaterial({...})` call keeps compiling
  and behaves identically (no normal map → vertex-normal path). Normal maps are
  loaded through the existing `loadTexture` as **linear `RGBA8`** (TextureLoader
  applies no sRGB), which is correct for direction data — no loader change.
- **TBN in the g-buffer geometry pass (`assets/shaders/mesh.glsl`).** The vertex
  stage passes the world-space tangent (`v_worldTangent = mat3(u_transform) *
  a_tangent`). The fragment stage builds an orthonormal TBN with Gram-Schmidt and
  perturbs the normal only when a map is bound:
  ```glsl
  vec3 N = normalize(v_worldNormal);
  if (u_hasNormal == 1) {
      vec3 T = normalize(v_worldTangent - dot(v_worldTangent, N) * N);
      mat3 TBN = mat3(T, cross(N, T), N);
      vec3 tn = texture(u_normalMap, v_uv).rgb * 2.0 - 1.0;
      N = normalize(TBN * tn);
  }
  gNormal = vec4(N, u_roughness);
  ```
  The lighting pass is untouched — it reads `gNormal` exactly as before.
- **Backend binding (`OpenGLBackend::executeGeometry`).** Mirrors the existing
  albedo binding: the normal map binds to **texture unit 1** (albedo stays on 0),
  and `u_hasNormal` is set from `mat.normalMap != nullptr`. Materials without a
  normal map set `u_hasNormal = 0` and take the vertex-normal path.

## Data flow

```
mesh (pos/normal/uv/tangent) + Material{albedo, normalMap}
  -> executeGeometry: bind albedo(0) + normalMap(1), set u_hasAlbedo/u_hasNormal
  -> mesh.glsl geometry pass: TBN * (normalMap*2-1) -> gNormal  (perturbed normal)
  -> [unchanged] SSAO / deferred PBR (PCF shadows, point + directional, IBL)
  -> bloom -> composite (ACES + gamma)
```

## `normalmap_demo`

`normalmapdemo::NormalMapDemoGame` (`normalmap_demo_game` target) is the engine's
first textured-material consumer. It draws two brick walls (scaled cubes) side by
side:

- **Left — flat:** `Material{ albedo = brick_d.jpg }`.
- **Right — normal-mapped:** `Material{ albedo = brick_d.jpg, normalMap = brick_n.jpg }`.

A single point light grazes the walls; `BOTARENA_LIGHT=0|1` freezes it at one of
two raking angles for deterministic before/after screenshots (it orbits live
otherwise). Camera-facing billboard labels ("flat" / "normal-mapped", reusing the
v0.33 world text) sit over each wall. Visual proof: the right wall shows per-brick
relief and mortar self-shadowing that the left wall lacks, and that relief tracks
the light as its angle changes — confirming a genuine per-fragment normal
perturbation rather than baked shading.

## Testing

The GL/shader path is screenshot-validated, consistent with the rest of the
deferred renderer (which has no unit tests for the g-buffer / PCF / SSAO passes):
the `normalmap_demo` at both light presets confirms the mapped wall shows relief
the flat wall does not and that it responds to the light direction. Regression is
covered by the existing games — their materials have `normalMap == nullptr`
(vertex-normal path), and the shooter/arena screenshots are visually unchanged
from before this slice. The full build and unit suite (`bot_arena_tests`) stay
green; the tangent attribute is added in lockstep across the cube, `ModelLoader`,
and `mesh.glsl`, so no mesh is fed a layout its shader doesn't expect.

## Scope for this slice

- **Tangent:** `vec3` only; the bitangent is derived in-shader. `vec4` tangent
  handedness (needed for mirrored UVs) is reserved — the cube and brick assets
  have no mirrored UVs.
- **Reserved, not implemented:** **parallax mapping** (the natural next slice on
  this same tangent seam; `height_map.png` / `waveHeightMap.png` assets are
  already present); per-material normal strength; a shared TextureLoader
  sRGB-vs-linear policy.

## Next Milestones

- Parallax / relief mapping using the same TBN and a height map.
- `vec4` tangent with handedness for mirrored-UV production models.
- Per-material normal-map strength.
