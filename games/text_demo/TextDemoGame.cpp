#include "games/text_demo/TextDemoGame.hpp"

#include <string>
#include <string_view>

#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
#include "engine/renderer/text/TextSpan.hpp"
#include "engine/renderer/text/TextStyle.hpp"

namespace textdemo {

namespace {

engine::TextPlacement at(float x, float y, float scale) {
  engine::TextPlacement p;
  p.pos = {x, y};
  p.scale = scale;
  return p;
}

engine::TextStyle fill(const glm::vec4& c) {
  engine::TextStyle s;
  s.fillColor = c;
  return s;
}

engine::FontHandle loadFont(
    engine::Renderer& r, const char* file, uint32_t px,
    engine::FontBackend backend = engine::FontBackend::Bitmap) {
  engine::FontDesc desc;
  desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/" + file;
  desc.pixelSize = px;
  desc.backend = backend;
  return r.fonts().load(desc);
}

}  // namespace

void TextDemoGame::onUpdate(float dt) { m_time += dt; }

void TextDemoGame::loadFonts(engine::Renderer& renderer) {
  if (!m_sans) m_sans = loadFont(renderer, "DejaVuSans.ttf", 32);
  if (!m_script) {
    m_script = loadFont(renderer, "Lobster-Regular.ttf", 48);
    if (!m_script)
      m_script = m_sans;  // clean checkout lacks the decorative face
  }
  if (!m_display) {
    m_display = loadFont(renderer, "BAUHS93.TTF", 44);
    if (!m_display) m_display = m_sans;
  }
  if (!m_sdf) {
    m_sdf = loadFont(renderer, "DejaVuSans.ttf", 48, engine::FontBackend::SDF);
  }
}

void TextDemoGame::onRender(engine::Renderer& renderer, int width, int height) {
  loadFonts(renderer);
  if (!m_sans) return;

  const glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
  const float h = static_cast<float>(height);

  const glm::vec4 ink{0.05f, 0.05f, 0.08f, 1.0f};

  // --- Effect column (left): real single-pass SDF effects on m_sdf ---
  if (m_sdf) {
    // Outline (white fill, dark ring).
    engine::TextStyle outline;
    outline.fillColor = white;
    outline.outlineColor = ink;
    outline.outlineWidthPx = 2.5f;
    renderer.drawText(m_sdf, "Outline", at(40.0f, 300.0f, 0.8f), outline);

    // Cartoon (bright fill, thick dark outline).
    engine::TextStyle cartoon;
    cartoon.fillColor = {1.0f, 0.82f, 0.15f, 1.0f};
    cartoon.outlineColor = {0.10f, 0.05f, 0.0f, 1.0f};
    cartoon.outlineWidthPx = 4.0f;
    renderer.drawText(m_sdf, "CARTOON", at(40.0f, 350.0f, 0.9f), cartoon);

    // Colored stroke (dark fill, bright ring).
    engine::TextStyle stroke;
    stroke.fillColor = ink;
    stroke.outlineColor = {1.0f, 0.25f, 0.55f, 1.0f};
    stroke.outlineWidthPx = 3.0f;
    renderer.drawText(m_sdf, "Colored stroke", at(40.0f, 405.0f, 0.8f), stroke);

    // Glow (white fill, cyan outer glow).
    engine::TextStyle glow;
    glow.fillColor = white;
    glow.glowColor = {0.3f, 0.85f, 1.0f, 1.0f};
    glow.glowSizePx = 9.0f;
    renderer.drawText(m_sdf, "Glow", at(40.0f, 460.0f, 0.9f), glow);

    // Drop shadow (white fill, soft offset shadow).
    engine::TextStyle shadow;
    shadow.fillColor = white;
    shadow.shadowColor = {0.0f, 0.0f, 0.0f, 0.7f};
    shadow.shadowOffsetPx = {3.0f, 3.0f};
    shadow.shadowSoftnessPx = 2.0f;
    renderer.drawText(m_sdf, "Drop shadow", at(40.0f, 515.0f, 0.8f), shadow);
  }

  // Header (kept clear of the top-left debug overlay).
  renderer.drawText(m_sans, "Text Effects Showcase", at(430.0f, 60.0f, 1.4f),
                    fill(white));
  renderer.drawText(m_sans, "single-pass SDF effects + multiple fonts",
                    at(430.0f, 92.0f, 0.7f), fill({0.75f, 0.8f, 0.9f, 1.0f}));

  // --- Font column (right) ---
  renderer.drawText(m_sans, "DejaVu Sans  AaBbCc 0123",
                    at(430.0f, 300.0f, 1.0f), fill(white));

  if (m_script) {
    renderer.drawText(m_script, "Lobster Script", at(430.0f, 360.0f, 1.0f),
                      fill({1.0f, 0.9f, 0.6f, 1.0f}));
  }
  if (m_display) {
    renderer.drawText(m_display, "BAUHAUS 93", at(430.0f, 425.0f, 1.0f),
                      fill(white));
  }

  // SDF vs bitmap at large scale: same target height (~96 px), bitmap upscaled
  // 3x (blurry) vs SDF baked-48 at 2x (crisp).
  const glm::vec4 label{0.15f, 0.2f, 0.3f, 1.0f};
  renderer.drawText(m_sans, "bitmap 3x", at(850.0f, 285.0f, 0.6f), fill(label));
  renderer.drawText(m_sans, "Aa 12", at(850.0f, 345.0f, 3.0f), fill(white));
  renderer.drawText(m_sans, "SDF 2x", at(850.0f, 430.0f, 0.6f), fill(label));
  if (m_sdf) {
    renderer.drawText(m_sdf, "Aa 12", at(850.0f, 490.0f, 2.0f), fill(white));
  }

  // Rich text: per-span styles in a single draw (color + glow + outline).
  if (m_sdf) {
    engine::TextStyle plain;
    plain.fillColor = white;
    engine::TextStyle red;
    red.fillColor = {1.0f, 0.35f, 0.3f, 1.0f};
    engine::TextStyle glow;
    glow.fillColor = white;
    glow.glowColor = {0.3f, 0.85f, 1.0f, 1.0f};
    glow.glowSizePx = 8.0f;
    engine::TextStyle outlined;
    outlined.fillColor = {1.0f, 0.85f, 0.2f, 1.0f};
    outlined.outlineColor = ink;
    outlined.outlineWidthPx = 2.5f;
    const engine::TextSpan rich[] = {{"Rich: ", plain},
                                     {"red ", red},
                                     {"glow ", glow},
                                     {"outline", outlined}};
    renderer.drawText(m_sdf, rich, at(430.0f, 240.0f, 0.7f));
  }

  // Footer: honest about the technique.
  renderer.drawText(
      m_sans,
      "Real single-pass SDF effects: outline, cartoon, stroke, glow, shadow "
      "(left). SDF stays crisp at any scale (right).",
      at(40.0f, h - 30.0f, 0.6f), fill({0.6f, 0.6f, 0.65f, 1.0f}));
}

}  // namespace textdemo
