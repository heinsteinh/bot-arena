#ifndef ENGINE_RENDERER_TEXT_TEXTSPAN_HPP
#define ENGINE_RENDERER_TEXT_TEXTSPAN_HPP

#include <string_view>

#include "engine/renderer/text/TextStyle.hpp"

namespace engine {

// One styled run of text. `text` is not retained past a synchronous submit.
struct TextSpan {
  std::string_view text;
  TextStyle style;
};

}  // namespace engine

#endif  // ENGINE_RENDERER_TEXT_TEXTSPAN_HPP
