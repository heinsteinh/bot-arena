#ifndef ENGINE_RENDERER_TEXT_SHELFPACKER_HPP
#define ENGINE_RENDERER_TEXT_SHELFPACKER_HPP

#include <algorithm>
#include <glm/glm.hpp>

namespace engine {

// Simple shelf (row) packer for a fixed-width atlas. Rows advance downward as
// they fill. Shared by static (offline/bitmap) and future dynamic atlases.
class ShelfPacker {
 public:
  explicit ShelfPacker(int width, int pad = 1)
      : m_width(width), m_pad(pad), m_penX(pad), m_penY(pad), m_rowH(0) {}

  // Reserve a w x h cell; returns its top-left pixel. Wraps to a new shelf if
  // the current row cannot fit the cell.
  glm::ivec2 place(int w, int h) {
    if (m_penX + w + m_pad > m_width) {
      m_penX = m_pad;
      m_penY += m_rowH + m_pad;
      m_rowH = 0;
    }
    const glm::ivec2 pos{m_penX, m_penY};
    m_penX += w + m_pad;
    m_rowH = std::max(m_rowH, h);
    return pos;
  }

  int width() const { return m_width; }
  int height() const { return m_penY + m_rowH + m_pad; }

 private:
  int m_width, m_pad;
  int m_penX, m_penY, m_rowH;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_SHELFPACKER_HPP
