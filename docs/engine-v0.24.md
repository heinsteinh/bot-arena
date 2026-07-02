# Engine v0.24 — Textured Materials

v0.24 loads `map_Kd` albedo textures for models, so a model with a diffuse texture
(e.g. `planet.obj` → `planet_Quom1200.png`) renders textured. This is the second half
of the material split — v0.23 added per-part colors, v0.24 adds the texture path.
Solid-material games (`bot_arena`/`arena_game`/`particles_game`) are unchanged.

See `docs/engine-v0.23.md` for the material colors this builds on.

## The pipeline

`.obj` + `.mtl` --Assimp--> per-submesh material (Kd color + map_Kd path)
`map_Kd` --resolveTexturePath--> path --loadTexture (stb_image)--> RGBA Texture2D
mesh vertices --+uv--> geometry pass samples `u_albedoMap` when the material has one.

## What changed

- **`Texture2D`** gained an `RGBA8` format alongside the R8 font atlas:
  `Create(w, h, TextureFormat = R8)`.
- **`loadTexture`** (`engine/assets/TextureLoader`) reads an image with `stb_image`
  (forced RGBA, **V-flipped** for OBJ UVs) into an RGBA8 `Texture2D`; null on failure.
- **UV vertex attribute:** the mesh layout is now `position+normal+uv`; the built-in
  cube gets per-face UVs, and `ModelLoader` includes each vertex's texcoord. The shadow
  pass (position-only) is unaffected, and solid materials render identically
  (`u_hasAlbedo` defaults to 0).
- **`mesh.glsl`** samples `albedo = u_hasAlbedo==1 ? texture(u_albedoMap, v_uv).rgb *
  u_baseColor.rgb : u_baseColor.rgb`.
- **`Material`** gained `Ref<Texture2D> albedo`; the geometry pass binds it to unit 0
  and sets `u_albedoMap`/`u_hasAlbedo` per material.
- **`ModelLoader`** reads each material's `aiTextureType_DIFFUSE` (`map_Kd`), resolves
  it against the model directory (`resolveTexturePath`), and loads it into the material
  alongside the v0.23 `Kd` color.

## The tested core

`engine/assets/TexturePath.hpp` — `resolveTexturePath(modelPath, textureRef)`: joins a
relative texture to the model's directory, normalizes `\\`→`/`, and passes absolute
paths through. Unit-tested. The texture upload / GL path is screenshot-verified.

## Demo

`assets/Objects/Planet/` (obj + mtl + `planet_Quom1200.png`, 1200×600) is vendored and
is the viewer's default. It renders as a sphere textured with its surface map — visible
proof of the albedo path. Color-only models (Monitor, Suzanne) keep their v0.23 look.

## Testing

- Unit (Catch2, pure): `resolveTexturePath` (relative join, nested path, absolute
  passthrough, backslash normalize, no-directory). 74 test cases total.
- Behavioral: `models_game` logs `Loaded texture .../planet_Quom1200.png (1200x600)`
  and the model; the screenshot shows the textured planet. `bot_arena` renders
  unchanged (its cube now has UVs but no albedo map — regression-checked).

## Next Milestones

- Normal / metallic / roughness / emissive maps; `Ns` → roughness.
- Mipmaps + anisotropic filtering; texture caching/dedup across materials.
- Use loaded models as props in the arena game.
