# Renderer v0.9 — Environment Cubemap & Skybox

v0.9 lays the foundation for image-based lighting: a `TextureCube` resource, a
procedural sky rendered into an environment cubemap, and that cubemap drawn as the
scene background (skybox). Reflections come in v0.10 — v0.9 makes the environment
*visible* and delivers the render-to-cubemap primitive that v0.10's convolution
passes reuse.

See `docs/renderer-v0.8.md` for the PBR lighting this builds on.

## Data Flow

```
Renderer ctor (once)
  │  m_envMap = TextureCube::Create(512, 1)
  │  backend.renderEnvironment(m_envMap->rendererID(), 512, m_lightDir)  // sky -> 6 faces
  │  backend.setEnvironment(m_envMap->rendererID())
  ▼
Renderer::endFrame (per frame; shadow -> geometry -> lighting -> composite)
  Lighting pass (fullscreen)   [Camera UBO binding 0 now carries invViewProjection]
    if (dot(N,N) < 0.5) {                          // background pixel
      clip  = vec4(v_uv*2-1, 1, 1)
      world = u_invViewProjection * clip
      dir   = normalize(world.xyz/world.w - u_cameraPos.xyz)
      fragColor = texture(u_envMap, dir)           // skybox
      return
    }
    ... PBR shading (unchanged from v0.8) ...
```

## TextureCube resource

`engine/renderer/TextureCube.hpp` + `opengl/OpenGLTextureCube.cpp`:

```cpp
static Ref<TextureCube> Create(uint32_t size, uint32_t mipLevels = 1);
void bind(uint32_t unit) const;
uint32_t rendererID() const;
```

`OpenGLTextureCube` allocates a `GL_TEXTURE_CUBE_MAP` (`RGBA16F`, all 6 faces via
`glTextureStorage2D`), `GL_LINEAR`, `GL_CLAMP_TO_EDGE` on S/T/R. The backend enables
`GL_TEXTURE_CUBE_MAP_SEAMLESS` once so cross-face sampling has no seams. The env map
is 512², 1 mip.

## Render-to-cubemap + procedural sky

`RenderBackend::renderEnvironment(cubemap, size, sunDir)` creates a temporary FBO
and, for each of the 6 faces, attaches it with
`glNamedFramebufferTextureLayer(fbo, COLOR_ATTACHMENT0, cubemap, 0, face)`, sets that
face's `forward`/`right`/`up` basis on the sky program, and draws the fullscreen
quad. The 6 bases follow the GL cubemap convention.

The **sky shader** reconstructs each fragment's world direction from the face basis
+ quad UV and outputs an HDR sky: a gradient (deep-blue zenith → pale horizon →
dark ground) plus a sharp sun disc (`pow(dot(dir, sunDir), 512) * 30`) and a soft
glow along `sunDir`. The Renderer passes `m_lightDir` as `sunDir`, so the skybox's
sun and the scene's directional light coincide.

`setEnvironment(cubemap)` records the id; `lightingPass` binds it to texture unit 4
(`samplerCube u_envMap`).

## Camera UBO — invViewProjection

To reconstruct a per-pixel world view ray in the fullscreen lighting pass,
`CameraUniforms` gained a trailing `mat4 invViewProjection` (`makeCameraUniforms`
sets it to `inverse(viewProjection)`). It is appended after `cameraPosition`, so the
earlier std140 offsets — and `mesh.glsl`'s Camera block, which does not declare it —
are unaffected.

## Skybox in the lighting pass

The lighting fragment shader's background branch (pixels where the G-buffer normal
is ~0) unprojects the pixel's NDC by `u_invViewProjection`, forms the world ray from
the camera, and samples `u_envMap`. Geometry pixels shade with the v0.8 PBR path,
unchanged. The metals do **not** yet reflect the sky — that is v0.10.

## Testing

- `test_camera_uniforms` (Catch2, no GL): `makeCameraUniforms` fills
  `invViewProjection`; `invViewProjection * viewProjection ≈ identity`.
- Behavioral: the `BOTARENA_SCREENSHOT` path shows a procedural sky (blue-to-pale
  gradient) filling the background behind the PBR-lit swarm. The sun is off-frame in
  the default downward camera view.

## Next Milestones

- **v0.10 — IBL:** irradiance convolution (diffuse ambient), prefiltered
  environment map + BRDF LUT (specular ambient), and swap the constant ambient for
  IBL — all reusing this milestone's `TextureCube` and `renderEnvironment`.
- Later: `.hdr` environment loading, a rotatable/animated sky, bloom/SSAO, light
  culling.
