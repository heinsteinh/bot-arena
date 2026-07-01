# Renderer v0.8 — PBR Materials (Analytic Cook-Torrance)

v0.8 makes the deferred lighting pass **physically based**. Materials gain
`metallic` and `roughness`; the lighting pass swaps Lambert diffuse for a
Cook-Torrance microfacet BRDF (GGX distribution, Smith geometry, Fresnel-Schlick)
over the directional and point lights. The two new scalars ride in the unused
G-buffer alpha channels, so no new attachment is needed. Ambient is a constant
(non-image-based) term; full IBL is a later milestone.

See `docs/renderer-v0.7.md` for the deferred G-buffer this builds on.

## Data Flow: One Frame

```
Material { baseColor, metallic, roughness, shader }
  │  executeGeometry sets u_baseColor, u_metallic, u_roughness
  ▼
Geometry pass -> G-buffer MRT
  gAlbedo   = vec4(baseColor.rgb, metallic)     (RGBA8)
  gNormal   = vec4(normalize(worldNormal), roughness)  (RGBA16F)
  gWorldPos = vec4(worldPos, 1)                 (RGBA16F)
  ▼
Lighting pass (fullscreen)   [reads Camera UBO binding 0 -> u_cameraPos]
  unpack albedo/metallic <- gAlbedo ; N/roughness <- gNormal ; worldPos <- gWorldPos
  V  = normalize(u_cameraPos.xyz - worldPos)
  F0 = mix(vec3(0.04), albedo, metallic)
  color = ambient (constant, Fresnel-weighted)
        + brdf(directional) * (1 - shadowPCF)
        + Σ brdf(point_i)
  fragColor = vec4(color, 1)                     (HDR)
  ▼
Composite -> tonemap (Reinhard + gamma), unchanged
```

The pass structure (shadow → geometry → lighting → composite) is unchanged from
v0.7; only the geometry writes and the lighting math changed.

## Materials

```cpp
struct Material {
  glm::vec4 baseColor{1.0f};
  float metallic = 0.0f;
  float roughness = 0.5f;
  ShaderHandle shader = 0;
};
```

`OpenGLBackend::executeGeometry` sets `u_metallic` / `u_roughness` alongside
`u_baseColor` per material.

## G-buffer packing

Metallic and roughness reuse the previously-unused alpha channels — no fourth
target, no extra bandwidth:

- `gAlbedo` (RGBA8): `.rgb` base colour, **`.a` metallic**.
- `gNormal` (RGBA16F): `.rgb` world normal, **`.a` roughness**.
- `gWorldPos` (RGBA16F): unchanged.

## Cook-Torrance BRDF

The lighting shader reads the camera position from the Camera UBO (binding 0) for
the view vector `V`, then for each light accumulates:

```
H = normalize(V + L)
D = GGX / Trowbridge-Reitz(N, H, roughness)          // a = roughness²
G = Smith Schlick-GGX(N, V, L, roughness)             // direct-light k = (rough+1)²/8
F = Fresnel-Schlick(dot(H,V), F0)                     // F0 = mix(0.04, albedo, metallic)
specular = D*G*F / (4*NoV*NoL + eps)
kD = (1 - F) * (1 - metallic)                         // metals have no diffuse
Lo += (kD*albedo/PI + specular) * radiance * NoL
```

- **Directional:** `radiance = vec3(1.0) * 3.0`, scaled by `(1 - shadowPCF)` (the
  3×3 PCF from v0.6, still evaluated here).
- **Point lights:** `radiance = color.rgb * intensity * attenuation`, with
  `attenuation = clamp(1 - d/radius, 0, 1)²`.

## Ambient (constant, no IBL)

A flat `albedo`-only ambient would leave metals black (metals have no diffuse and,
without an environment, nothing to reflect). Instead the ambient is a **constant
Fresnel-weighted** term so metals get a tinted ambient reflection:

```glsl
vec3 Famb  = fresnelSchlick(max(dot(N, V), 0.0), F0);
vec3 kdAmb = (1.0 - Famb) * (1.0 - metallic);
vec3 ambient = (kdAmb * albedo + Famb) * 0.12;
```

This is still analytic — a constant grey ambient, not image-based lighting. Real
directional ambient reflections (IBL) are a later milestone.

## Scene materials (the demo)

`BotArenaGame` registers a spread so the range is visible:

| Material | baseColor | metallic | roughness |
| --- | --- | --- | --- |
| Ground | `(0.35,0.35,0.38)` | 0.0 | 0.85 |
| Walls | `(0.7,0.7,0.7)` | 0.0 | 0.5 |
| Swarm 0 — polished gold metal | `(1.0,0.78,0.34)` | 1.0 | 0.25 |
| Swarm 1 — brushed steel-blue metal | `(0.6,0.7,0.9)` | 1.0 | 0.45 |
| Swarm 2 — glossy red plastic | `(0.85,0.28,0.24)` | 0.0 | 0.30 |

Metal cubes show tinted reflections and tight specular glints tracking the orbiting
point lights; the plastic cubes show diffuse red with broader soft highlights; the
ground stays matte.

## Testing

- `test_resource_registry` (Catch2, no GL): `metallic`/`roughness` round-trip
  through `registerMaterial`/`material`, and defaults are `metallic 0`,
  `roughness 0.5`. The BRDF math lives in-shader and is not duplicated CPU-side.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows the swarm with
  PBR-differentiated surfaces (metal vs plastic vs matte ground) under the
  shadowed directional light and coloured point lights, tonemapped.

## Next Milestones

- **Image-based lighting** (irradiance map + prefiltered environment + BRDF LUT)
  for real ambient reflections — replaces the constant ambient here.
- **Texture maps** (albedo / normal / metallic-roughness / AO) instead of flat
  per-material scalars.
- **AO and emissive** G-buffer channels.
- **Light culling** (tiled / clustered) and **bloom / SSAO**.
