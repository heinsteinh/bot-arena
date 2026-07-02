# Engine v0.22 — Assimp Model Loading & Viewer

v0.22 adds Assimp and imports real `.obj` models into the engine's mesh path
(position+normal), with a `models_game` viewer that loads a curated set and lets you
pick between them with an ImGui panel. Models render with solid-color materials for now
(the deferred renderer shades per-material colors, not textures). `bot_arena`,
`arena_game`, and `particles_game` are untouched.

See `docs/engine-v0.20.md` / `docs/engine-v0.21.md` for the mesh path and ImGui this
builds on.

## Loading

`engine/assets/ModelLoader.loadModel(path, registry) -> Model{ mesh, bounds, valid }`:

- Assimp `ReadFile` with `aiProcess_Triangulate | aiProcess_GenSmoothNormals |
  aiProcess_JoinIdenticalVertices | aiProcess_PreTransformVertices` (triangulate,
  generate missing normals, weld verts, and bake the node hierarchy into world space).
- All sub-meshes are merged into one interleaved **position+normal** vertex buffer +
  index buffer (indices offset by the running vertex base), built as a `VertexArray`
  with the engine's existing layout and registered via `registerMesh`. Missing normals
  fall back to `(0,1,0)`.
- On failure (unreadable / incomplete / no geometry) it logs the importer error and
  returns `{valid = false}`.
- **Materials/`.mtl`/textures are ignored** — models render with an engine solid-color
  material. Textured PBR is a follow-up (needs an RGBA texture path in the deferred
  shader).

## The tested core

`engine/assets/MeshBounds.hpp` (pure, no Assimp/GL):

- `computeBounds(points, count)` → the model's `AABB`.
- `fitToUnitTransform(AABB)` → a matrix that centers the model at the origin and
  uniform-scales it so its largest extent is 1 — so a huge statue and a small teapot
  view at the same size.

## The viewer (`games/models/`)

`models_game` (fourth executable, linking `bot_arena_engine`) loads a curated set
(Suzanne, Teapot, Sphere, Torus, Spaceship, Statue) on the first `onRender`, keeps the
valid ones, and renders the selected model as `fitToUnitTransform(bounds) *
rotate(angle)` with a solid material on a ground plane, under an orbit camera. Its
`onImGuiRender` (v0.21) is a "Model Viewer" panel — a radio list of models (failed
loads show `(failed)`) plus an auto-rotate toggle.

## Assets

The six curated `.obj` files (and their `.mtl` siblings) are vendored under
`assets/meshes/`. The other model/texture files added under `assets/` remain untracked
— vendor whichever you want to keep in the repo.

## Testing

- Unit (Catch2, no Assimp/GL): `computeBounds` (min/max, empty → zero) and
  `fitToUnitTransform` (centers + unit-scales a known box). 69 test cases total.
- Behavioral: `models_game` logs `Loaded model ...` for each curated file; the
  screenshot shows a loaded, centered model (Suzanne) with the picker panel, and
  selecting another (Torus) swaps it. Other games unchanged.

## Next Milestones

- Textured/`.mtl` PBR materials (RGBA `Texture2D` + sampling in the mesh shader),
  glTF/FBX imports, instanced model rendering, and mesh LOD.
- Use the loaded models in the actual games (e.g. bots/props in the arena).
