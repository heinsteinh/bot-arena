#include "games/text_demo/TextDemoGame.hpp"

#include <cmath>
#include <string>

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
engine::TextStyle fill(float r, float g, float b, float a = 1.0f) {
  engine::TextStyle s;
  s.fillColor = {r, g, b, a};
  return s;
}
}  // namespace

void TextDemoGame::onUpdate(float dt) { m_time += dt; }

void TextDemoGame::onRender(engine::Renderer& renderer, int width, int height) {
  if (!m_font) {
    engine::FontDesc desc;
    desc.family = std::string(BOTARENA_ASSET_DIR) + "/fonts/DejaVuSans.ttf";
    desc.pixelSize = 32;
    desc.backend = engine::FontBackend::Bitmap;
    m_font = renderer.fonts().load(desc);
  }
  if (!m_font) return;

  // Title.
  renderer.drawText(m_font, "Bot Arena Text System", at(40.0f, 80.0f, 1.4f),
                    fill(1.0f, 1.0f, 1.0f));

  // Color swatches at different scales.
  renderer.drawText(m_font, "Red fill", at(40.0f, 150.0f, 1.0f),
                    fill(1.0f, 0.3f, 0.3f));
  renderer.drawText(m_font, "Green fill", at(40.0f, 190.0f, 1.0f),
                    fill(0.3f, 1.0f, 0.4f));
  renderer.drawText(m_font, "Blue fill (small)", at(40.0f, 224.0f, 0.7f),
                    fill(0.5f, 0.7f, 1.0f));

  // Animated pulsing alpha.
  const float pulse = 0.5f + 0.5f * std::sin(m_time * 3.0f);
  renderer.drawText(m_font, "Pulsing", at(40.0f, 270.0f, 1.1f),
                    fill(1.0f, 0.85f, 0.2f, pulse));

  // Counter.
  const int ticks = static_cast<int>(m_time * 10.0f);
  renderer.drawText(m_font, "Ticks: " + std::to_string(ticks),
                    at(40.0f, 320.0f, 0.9f), fill(0.8f, 0.8f, 0.9f));

  // Bottom hint.
  renderer.drawText(m_font, "Slice 1: bitmap backend. SDF/outline/glow next.",
                    at(40.0f, static_cast<float>(height) - 40.0f, 0.6f),
                    fill(0.6f, 0.6f, 0.65f));
}

}  // namespace textdemo
