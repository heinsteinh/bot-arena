# Engine v0.23 — Model Material Colors

v0.23 reads each sub-mesh's diffuse color (`Kd`) from the `.mtl` via Assimp, so
multi-material models render with their real per-part colors instead of one merged
grey. This is the first half of the material split — textured (`map_Kd`) albedo maps
come in v0.24.

See `docs/engine-v0.22.md` for the Assimp loading this refines.

## What changed

`loadModel` no longer merges every sub-mesh into one mesh. Instead:

- `Model` is now `{ std::vector<Submesh> submeshes; AABB bounds; bool valid; }`, where
  `Submesh = { MeshHandle mesh; MaterialHandle material; }`.
- For each `aiMesh`: a position+normal `VertexArray` → `registerMesh`, and its Assimp
  material's diffuse color (`AI_MATKEY_COLOR_DIFFUSE`, grey `0.8` fallback) →
  `registerMaterial({color, metallic 0, roughness 0.55, shader})`.
- Bounds are computed over all sub-mesh positions.
- Signature: `loadModel(path, registry, shader)` — the mesh shader handle is needed to
  register the materials.

The viewer submits **every** sub-mesh with its own material under the one model
transform (`fitToUnitTransform(bounds) * rotate`). The vertex layout, `mesh.glsl`, and
the texture path are unchanged.

## Demo

`monitor.obj` (multi-material: a red car-paint body/stand and a green screen) is
vendored and is the viewer's default. It loads as 2 sub-meshes and renders with those
two colors — visible proof that per-part `.mtl` colors are applied. Single-material
models (Suzanne, teapot, …) load as one sub-mesh and render as before.

## Testing

Integration/asset milestone — verified by screenshot; no new pure unit (the
`MeshBounds` tests and the v0.22 loader path stand). 69 test cases total.

- Behavioral: `models_game` logs `Loaded model .../monitor.obj (2 submeshes, ...)`;
  the screenshot shows the Monitor with a red frame and green screen (not grey), plus
  the picker. Other games unchanged.

## Next Milestones

- **v0.24 — Textured materials:** a UV vertex attribute, an RGBA `Texture2D` +
  `stb_image` loader, `mesh.glsl` albedo sampling, and reading `map_Kd` from Assimp —
  so textured models (Planet, Car) show their maps.
- Later: normal/metallic/roughness maps, per-material `Ns` → roughness.
