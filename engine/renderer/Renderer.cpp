#include "engine/renderer/Renderer.hpp"

#include <spdlog/spdlog.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "engine/core/AssetPath.hpp"
#include "engine/renderer/Buffer.hpp"
#include "engine/renderer/Shader.hpp"
#include "engine/renderer/Texture2D.hpp"
#include "engine/renderer/VertexArray.hpp"
#include "engine/renderer/text/BitmapFreeTypeSource.hpp"
#include "engine/renderer/text/SdfFreeTypeSource.hpp"

namespace engine {

Renderer::Renderer(JobSystem& jobs)
    : m_jobs(jobs), m_backend(RenderBackend::Create()) {
  m_lanes.reserve(jobs.workerCount());
  for (size_t i = 0; i < jobs.workerCount(); ++i) {
    m_lanes.push_back(CreateScope<WorkerBuffer>(kLaneArenaBytes));
  }
  initBuiltins();

  m_sceneFBO = Framebuffer::Create(FramebufferSpec{});
  m_scenePass = RenderPass{m_sceneFBO, {0.08f, 0.09f, 0.11f, 1.0f}, true};
  m_compositePass = RenderPass{nullptr, {0.0f, 0.0f, 0.0f, 1.0f}, false};

  FramebufferSpec gbufferSpec;
  gbufferSpec.colorFormats = {
      FramebufferFormat::RGBA8,    // gAlbedo (rgb + metallic)
      FramebufferFormat::RGBA16F,  // gNormal (rgb + roughness)
      FramebufferFormat::RGBA16F,  // gWorldPos (xyz + geometry mask)
      FramebufferFormat::RGBA8};   // gShadow (parallax self-shadow)
  m_gbufferFBO = Framebuffer::Create(gbufferSpec);
  m_gbufferPass = RenderPass{m_gbufferFBO, {0.0f, 0.0f, 0.0f, 0.0f}, true};

  FramebufferSpec shadowSpec;
  shadowSpec.width = kShadowSize;
  shadowSpec.height = kShadowSize;
  shadowSpec.depthOnly = true;
  for (int i = 0; i < 3; ++i) m_shadowFBO[i] = Framebuffer::Create(shadowSpec);

  m_envMap = TextureCube::Create(kEnvSize, 1);
  m_backend->renderEnvironment(m_envMap->rendererID(),
                               static_cast<int>(kEnvSize), m_lightDir);
  m_backend->setEnvironment(m_envMap->rendererID());

  m_irradianceMap = TextureCube::Create(kIrradianceSize, 1);
  m_prefilterMap = TextureCube::Create(kPrefilterSize, kPrefilterMips);
  FramebufferSpec brdfSpec;
  brdfSpec.width = kBrdfSize;
  brdfSpec.height = kBrdfSize;
  m_brdfFBO = Framebuffer::Create(brdfSpec);

  m_backend->convolveIrradiance(m_envMap->rendererID(),
                                m_irradianceMap->rendererID(),
                                static_cast<int>(kIrradianceSize));
  m_backend->prefilterEnvironment(
      m_envMap->rendererID(), m_prefilterMap->rendererID(),
      static_cast<int>(kPrefilterSize), kPrefilterMips);
  m_backend->beginPass(m_brdfFBO.get(), {0.0f, 0.0f, 0.0f, 1.0f}, false,
                       static_cast<int>(kBrdfSize),
                       static_cast<int>(kBrdfSize));
  m_backend->integrateBRDF();
  m_backend->setIBL(m_irradianceMap->rendererID(), m_prefilterMap->rendererID(),
                    m_brdfFBO->colorAttachment(), kPrefilterMips);

  FramebufferSpec ssaoSpec;
  ssaoSpec.colorFormats = {FramebufferFormat::RGBA8};
  m_ssaoFBO = Framebuffer::Create(ssaoSpec);
  m_ssaoBlurFBO = Framebuffer::Create(ssaoSpec);

  FramebufferSpec bloomSpec;
  bloomSpec.colorFormats = {FramebufferFormat::RGBA16F};
  m_bloomFBO[0] = Framebuffer::Create(bloomSpec);
  m_bloomFBO[1] = Framebuffer::Create(bloomSpec);
}

void Renderer::initBuiltins() {
  // Unit cube: 6 faces x 4 verts, position(3) + normal(3) + uv(2). Each face's
  // verts get UVs (0,0),(1,0),(1,1),(0,1).
  const float v[] = {// +X
                     1, -1, -1, 1, 0, 0, 0, 0, 1, 1, -1, 1, 0, 0, 1, 0, 1, 1, 1,
                     1, 0, 0, 1, 1, 1, -1, 1, 1, 0, 0, 0, 1,
                     // -X
                     -1, -1, 1, -1, 0, 0, 0, 0, -1, 1, 1, -1, 0, 0, 1, 0, -1, 1,
                     -1, -1, 0, 0, 1, 1, -1, -1, -1, -1, 0, 0, 0, 1,
                     // +Y
                     -1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, -1,
                     0, 1, 0, 1, 1, -1, 1, -1, 0, 1, 0, 0, 1,
                     // -Y
                     -1, -1, -1, 0, -1, 0, 0, 0, 1, -1, -1, 0, -1, 0, 1, 0, 1,
                     -1, 1, 0, -1, 0, 1, 1, -1, -1, 1, 0, -1, 0, 0, 1,
                     // +Z
                     -1, -1, 1, 0, 0, 1, 0, 0, 1, -1, 1, 0, 0, 1, 1, 0, 1, 1, 1,
                     0, 0, 1, 1, 1, -1, 1, 1, 0, 0, 1, 0, 1,
                     // -Z
                     1, -1, -1, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0, -1, 1, 0, -1,
                     1, -1, 0, 0, -1, 1, 1, 1, 1, -1, 0, 0, -1, 0, 1};
  uint32_t idx[36];
  for (uint32_t f = 0; f < 6; ++f) {
    const uint32_t o = f * 4;
    const uint32_t b = f * 6;
    idx[b + 0] = o + 0;
    idx[b + 1] = o + 1;
    idx[b + 2] = o + 2;
    idx[b + 3] = o + 2;
    idx[b + 4] = o + 3;
    idx[b + 5] = o + 0;
  }

  // Append a per-face tangent (the +U direction) -> 11 floats/vertex.
  const float faceTan[6][3] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0},
                               {1, 0, 0}, {1, 0, 0},  {-1, 0, 0}};
  float vt[24 * 11];
  for (int f = 0; f < 6; ++f) {
    for (int c = 0; c < 4; ++c) {
      const int s = (f * 4 + c) * 8;
      const int d = (f * 4 + c) * 11;
      for (int k = 0; k < 8; ++k) vt[d + k] = v[s + k];
      vt[d + 8] = faceTan[f][0];
      vt[d + 9] = faceTan[f][1];
      vt[d + 10] = faceTan[f][2];
    }
  }

  auto va = VertexArray::Create();
  auto vb = VertexBuffer::Create(vt, sizeof(vt));
  vb->setLayout({
      {ShaderDataType::Float3, "a_position"},
      {ShaderDataType::Float3, "a_normal"},
      {ShaderDataType::Float2, "a_uv"},
      {ShaderDataType::Float3, "a_tangent"},
  });
  va->addVertexBuffer(vb);
  va->setIndexBuffer(IndexBuffer::Create(idx, 36));

  m_cubeMesh = m_registry.registerMesh(va);
  m_meshShader =
      m_registry.registerShader(Shader::Create(assetPath("shaders/mesh.glsl")));

  m_fonts = CreateScope<FontManager>([](const BakedFont& baked) {
    Ref<Texture2D> tex = Texture2D::Create(
        static_cast<uint32_t>(baked.atlasWidth),
        static_cast<uint32_t>(baked.atlasHeight), TextureFormat::R8);
    tex->setData(baked.atlasPixels.data(),
                 static_cast<uint32_t>(baked.atlasPixels.size()));
    return CreateRef<GlyphAtlas>(tex, baked.atlasWidth, baked.atlasHeight);
  });
  m_fonts->registerSource(CreateScope<BitmapFreeTypeSource>());
  m_fonts->registerSource(CreateScope<SdfFreeTypeSource>());
}

void Renderer::beginFrame(int width, int height) {
  m_width = width;
  m_height = height;
  for (Scope<WorkerBuffer>& lane : m_lanes) {
    lane->arena.reset();
    lane->queue.clear();
  }
  m_sceneFBO->resize(static_cast<uint32_t>(width),
                     static_cast<uint32_t>(height));
  m_gbufferFBO->resize(static_cast<uint32_t>(width),
                       static_cast<uint32_t>(height));
  m_ssaoFBO->resize(static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height));
  m_ssaoBlurFBO->resize(static_cast<uint32_t>(width),
                        static_cast<uint32_t>(height));
  const uint32_t halfW = static_cast<uint32_t>(width) / 2;
  const uint32_t halfH = static_cast<uint32_t>(height) / 2;
  m_bloomFBO[0]->resize(halfW, halfH);
  m_bloomFBO[1]->resize(halfW, halfH);
  m_textRenderer.clear();
  m_particleInstances.clear();
}

void Renderer::submitParticles(const std::vector<ParticleInstance>& instances) {
  m_particleInstances.insert(m_particleInstances.end(), instances.begin(),
                             instances.end());
}

void Renderer::drawText(const FontHandle& font, std::string_view text,
                        const TextPlacement& placement,
                        const TextStyle& style) {
  if (!font) return;
  m_textRenderer.submit(*font, text, placement, style, m_width, m_height);
}

void Renderer::drawText(const FontHandle& font, std::span<const TextSpan> spans,
                        const TextPlacement& placement) {
  if (!font) return;
  m_textRenderer.submit(*font, spans, placement, m_width, m_height);
}

Renderer::RenderStats Renderer::stats() const {
  RenderStats s;
  s.drawCount = m_merged.size();
  s.pointLights = m_pointLightCount;
  s.laneCount = m_lanes.size();
  s.cameraPos = glm::vec3(m_camera.cameraPosition);
  s.cameraFwd = glm::normalize(-glm::vec3(glm::inverse(m_camera.view)[2]));
  return s;
}

void Renderer::generateMeshes(
    size_t count, const std::function<void(RenderQueue&, size_t, size_t)>& fn) {
  m_jobs.parallelFor(count, kBatchSize,
                     [this, &fn](size_t begin, size_t end, size_t lane) {
                       fn(m_lanes[lane]->queue, begin, end);
                     });
}

void Renderer::endFrame() {
  m_merged.clear();
  mergeLaneEntries(m_lanes, m_merged);
  std::stable_sort(m_merged.begin(), m_merged.end(),
                   [](const RenderEntry& a, const RenderEntry& b) {
                     return a.sortKey < b.sortKey;
                   });

  // Nearest-N: sort point lights by camera distance so indices 0..2 (which get
  // parallax self-shadows) are the three nearest the camera.
  {
    std::vector<PointLight> sorted = m_pointLights;
    const glm::vec3 camPos = glm::vec3(m_camera.cameraPosition);
    std::stable_sort(
        sorted.begin(), sorted.end(),
        [&](const PointLight& a, const PointLight& b) {
          const glm::vec3 da = glm::vec3(a.positionRadius) - camPos;
          const glm::vec3 db = glm::vec3(b.positionRadius) - camPos;
          return glm::dot(da, da) < glm::dot(db, db);
        });
    m_backend->setPointLights(static_cast<int>(sorted.size()), sorted.data());
  }

  // Shadow pass -> depth-only shadow map (light POV).
  // Cascaded shadow maps: split the camera frustum by depth, fit + render a
  // tight depth map per cascade.
  const float kShadowNear = 0.5f, kShadowFar = 60.0f;
  float splits[3];
  for (int i = 0; i < 3; ++i) {
    const float p = static_cast<float>(i + 1) / 3.0f;
    const float logS = kShadowNear * std::pow(kShadowFar / kShadowNear, p);
    const float uniS = kShadowNear + (kShadowFar - kShadowNear) * p;
    splits[i] = glm::mix(uniS, logS, 0.5f);
  }
  float nearD = kShadowNear;
  for (int i = 0; i < 3; ++i) {
    const glm::mat4 vp = makeCascadeViewProj(
        m_lightDir, m_camera.view, m_camera.projection, nearD, splits[i]);
    m_light.cascadeViewProj[i] = vp;
    m_backend->beginPass(m_shadowFBO[i].get(), {0.0f, 0.0f, 0.0f, 1.0f}, true,
                         kShadowSize, kShadowSize);
    m_backend->executeShadow(m_merged, vp, m_lanes[0]->arena, m_registry);
    nearD = splits[i];
  }
  m_light.cascadeSplits = glm::vec4(splits[0], splits[1], splits[2], 0.0f);
  m_light.lightDir = glm::vec4(glm::normalize(m_lightDir), 0.0f);
  m_backend->setLight(m_light, m_shadowFBO[0]->depthAttachment(),
                      m_shadowFBO[1]->depthAttachment(),
                      m_shadowFBO[2]->depthAttachment());

  // Geometry pass -> G-buffer MRT (albedo/normal/world-pos + depth).
  m_backend->beginPass(m_gbufferPass.target.get(), m_gbufferPass.clearColor,
                       m_gbufferPass.clearDepth, m_width, m_height);
  m_backend->executeGeometry(m_merged, m_camera, m_lanes[0]->arena, m_registry);

  // SSAO pass -> ambient occlusion (reads the G-buffer normal + world-pos).
  m_backend->beginPass(m_ssaoFBO.get(), {1.0f, 1.0f, 1.0f, 1.0f}, false,
                       m_width, m_height);
  m_backend->ssaoPass(m_gbufferFBO->colorAttachment(1),
                      m_gbufferFBO->colorAttachment(2));
  m_backend->beginPass(m_ssaoBlurFBO.get(), {1.0f, 1.0f, 1.0f, 1.0f}, false,
                       m_width, m_height);
  m_backend->ssaoBlur(m_ssaoFBO->colorAttachment());
  m_backend->setAO(m_ssaoBlurFBO->colorAttachment());

  // Lighting pass -> HDR scene framebuffer (fullscreen, reads the G-buffer).
  m_backend->beginPass(m_scenePass.target.get(), {0.0f, 0.0f, 0.0f, 1.0f},
                       false, m_width, m_height);
  m_backend->lightingPass(
      m_gbufferFBO->colorAttachment(0), m_gbufferFBO->colorAttachment(1),
      m_gbufferFBO->colorAttachment(2), m_gbufferFBO->colorAttachment(3));

  // Emissive point-light billboards -> HDR scene (additive), before bloom.
  m_backend->drawLightBillboards(m_pointLightCount,
                                 m_gbufferFBO->colorAttachment(2));

  // Particles -> HDR scene (additive), before bloom, so they glow.
  m_backend->drawParticles(m_particleInstances.data(),
                           static_cast<int>(m_particleInstances.size()));

  // Bloom: bright-pass the HDR scene, then ping-pong separable Gaussian blur.
  const int halfW = m_width / 2;
  const int halfH = m_height / 2;
  m_backend->beginPass(m_bloomFBO[0].get(), {0.0f, 0.0f, 0.0f, 1.0f}, false,
                       halfW, halfH);
  m_backend->bloomExtract(m_sceneFBO->colorAttachment());
  int src = 0;
  bool horizontal = true;
  for (int i = 0; i < kBloomBlurPasses * 2; ++i) {
    const int dst = 1 - src;
    m_backend->beginPass(m_bloomFBO[dst].get(), {0.0f, 0.0f, 0.0f, 1.0f}, false,
                         halfW, halfH);
    m_backend->bloomBlur(m_bloomFBO[src]->colorAttachment(), horizontal);
    src = dst;
    horizontal = !horizontal;
  }

  // Composite pass -> default framebuffer (scene + bloom, tonemap).
  m_backend->beginPass(m_compositePass.target.get(), m_compositePass.clearColor,
                       m_compositePass.clearDepth, m_width, m_height);
  m_backend->compositeBloom(m_sceneFBO->colorAttachment(),
                            m_bloomFBO[src]->colorAttachment());

  // World-space billboard text -> default framebuffer, on top of the scene
  // but under the screen-space overlay.
  for (const TextRenderer::WorldBatch& b : m_textRenderer.worldBatches()) {
    m_backend->drawWorldTextBatch(b.atlas, b.verts, b.pxRange, b.styles);
  }

  // Text overlay -> default framebuffer (still bound), on top of the scene.
  for (const TextRenderer::Batch& b : m_textRenderer.batches()) {
    m_backend->drawTextBatch(b.backend, b.atlas, b.verts, b.pxRange, b.styles);
  }
}

void Renderer::setPointLights(const std::vector<PointLight>& lights) {
  m_pointLights = lights;
  const int count = static_cast<int>(lights.size());
  m_pointLightCount = count < 32 ? count : 32;
}

void Renderer::saveScreenshot(const std::string& path, int width, int height) {
  std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
  m_backend->readPixels(0, 0, width, height, pixels.data());

  stbi_flip_vertically_on_write(1);
  if (stbi_write_png(path.c_str(), width, height, 3, pixels.data(),
                     width * 3)) {
    spdlog::info("Screenshot saved to {}", path);
  } else {
    spdlog::error("Failed to write screenshot to {}", path);
  }
}

}  // namespace engine
