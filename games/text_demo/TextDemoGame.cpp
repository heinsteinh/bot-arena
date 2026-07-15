#include "games/text_demo/TextDemoGame.hpp"

#include <cmath>
#include <string>
#include <string_view>

#include "engine/renderer/Renderer.hpp"
#include "engine/renderer/text/FontDesc.hpp"
#include "engine/renderer/text/TextPlacement.hpp"
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

engine::FontHandle loadFont(engine::Renderer& r, const char* file,
                            uint32_t px) {
  engine::FontDesc desc;
  desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/" + file;
  desc.pixelSize = px;
  desc.backend = engine::FontBackend::Bitmap;
  return r.fonts().load(desc);
}

// Drop shadow: draw the string once offset in a dark colour, then the fill on
// top. Two draws, no engine effect support needed.
void drawShadow(engine::Renderer& r, const engine::FontHandle& f,
                std::string_view text, float x, float y, float scale,
                const glm::vec4& fillColor, const glm::vec4& shadowColor,
                float dx, float dy) {
  r.drawText(f, text, at(x + dx, y + dy, scale), fill(shadowColor));
  r.drawText(f, text, at(x, y, scale), fill(fillColor));
}

// Outline / stroke: draw the string in the outline colour at eight offsets
// around the pen, then the fill on top. A thick, dark outline reads as a
// "cartoon" look; a bright outline reads as a coloured stroke.
void drawOutline(engine::Renderer& r, const engine::FontHandle& f,
                 std::string_view text, float x, float y, float scale,
                 const glm::vec4& fillColor, const glm::vec4& lineColor,
                 float w) {
  static const float ox[8] = {-1, 1, 0, 0, -1, 1, -1, 1};
  static const float oy[8] = {0, 0, -1, 1, -1, -1, 1, 1};
  for (int i = 0; i < 8; ++i) {
    r.drawText(f, text, at(x + ox[i] * w, y + oy[i] * w, scale),
               fill(lineColor));
  }
  r.drawText(f, text, at(x, y, scale), fill(fillColor));
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
}

void TextDemoGame::onRender(engine::Renderer& renderer, int width, int height) {
  loadFonts(renderer);
  if (!m_sans) return;

  const glm::vec4 white{1.0f, 1.0f, 1.0f, 1.0f};
  const glm::vec4 ink{0.05f, 0.05f, 0.08f, 1.0f};
  const float h = static_cast<float>(height);

  // Header (kept clear of the top-left debug overlay).
  drawShadow(renderer, m_sans, "Text Effects Showcase", 430.0f, 60.0f, 1.4f,
             white, {0.0f, 0.0f, 0.0f, 0.6f}, 2.0f, 2.0f);
  renderer.drawText(m_sans, "bitmap backend | 3 fonts | layered effects",
                    at(430.0f, 92.0f, 0.7f), fill({0.75f, 0.8f, 0.9f, 1.0f}));

  // --- Effect column (left, below the debug overlay) ---
  drawShadow(renderer, m_sans, "Drop shadow", 40.0f, 300.0f, 1.2f, white,
             {0.0f, 0.0f, 0.0f, 0.7f}, 3.0f, 3.0f);

  drawOutline(renderer, m_sans, "Outline", 40.0f, 350.0f, 1.2f, white, ink,
              2.0f);

  // Cartoon: thick dark outline + bright saturated fill.
  drawOutline(renderer, m_sans, "CARTOON", 40.0f, 405.0f, 1.3f,
              {1.0f, 0.82f, 0.15f, 1.0f}, {0.10f, 0.05f, 0.0f, 1.0f}, 3.0f);

  // Coloured stroke: bright outline, dark fill.
  drawOutline(renderer, m_sans, "Colored stroke", 40.0f, 460.0f, 1.2f, ink,
              {1.0f, 0.25f, 0.55f, 1.0f}, 2.0f);

  // Pulsing glow-ish: outline whose alpha breathes.
  const float pulse = 0.35f + 0.35f * std::sin(m_time * 3.0f);
  drawOutline(renderer, m_sans, "Pulsing glow", 40.0f, 515.0f, 1.2f, white,
              {0.3f, 0.85f, 1.0f, pulse}, 3.0f);

  // --- Font column (right) ---
  renderer.drawText(m_sans, "DejaVu Sans  AaBbCc 0123",
                    at(430.0f, 300.0f, 1.0f), fill(white));

  if (m_script) {
    drawShadow(renderer, m_script, "Lobster Script", 430.0f, 360.0f, 1.0f,
               {1.0f, 0.9f, 0.6f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.6f}, 2.0f, 2.0f);
  }
  if (m_display) {
    drawOutline(renderer, m_display, "BAUHAUS 93", 430.0f, 425.0f, 1.0f, white,
                ink, 2.0f);
  }

  // Effects applied to the decorative faces.
  if (m_script) {
    drawOutline(renderer, m_script, "Sweet!", 430.0f, 490.0f, 1.2f,
                {1.0f, 0.45f, 0.7f, 1.0f}, {0.15f, 0.0f, 0.08f, 1.0f}, 3.0f);
  }
  if (m_display) {
    drawShadow(renderer, m_display, "HEAVY", 640.0f, 490.0f, 1.2f,
               {0.4f, 0.9f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 0.7f}, 4.0f, 4.0f);
  }

  // Footer: honest about the technique.
  renderer.drawText(
      m_sans,
      "Effects layered on the bitmap backend; crisp single-pass SDF/MSDF "
      "outline & glow arrive in Slices 2-3.",
      at(40.0f, h - 30.0f, 0.6f), fill({0.6f, 0.6f, 0.65f, 1.0f}));
}

}  // namespace textdemo
