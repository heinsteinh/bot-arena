#ifndef ENGINE_CORE_INPUT_HPP
#define ENGINE_CORE_INPUT_HPP

#include <glm/glm.hpp>
#include <unordered_map>

namespace engine {

enum class Key { Unknown, Escape, W, A, S, D, Q, E, Space, LeftShift };

enum class MouseButton { Left, Right, Middle };

class Input {
 public:
  static void beginFrame();

  static void setKey(Key key, bool down);
  static bool isKeyDown(Key key);
  static bool wasKeyPressed(Key key);

  static void setMouseButton(MouseButton button, bool down);
  static bool isMouseButtonDown(MouseButton button);

  static void setMousePosition(float x, float y);
  static void setMouseDelta(float dx, float dy);
  static void setScrollDelta(float dx, float dy);

  static glm::vec2 mousePosition();
  static glm::vec2 mouseDelta();
  static glm::vec2 scrollDelta();

 private:
  static std::unordered_map<Key, bool> s_keys;
  static std::unordered_map<Key, bool> s_previousKeys;

  static std::unordered_map<MouseButton, bool> s_mouseButtons;

  static glm::vec2 s_mousePosition;
  static glm::vec2 s_mouseDelta;
  static glm::vec2 s_scrollDelta;
};

}  // namespace engine

#endif  // ENGINE_CORE_INPUT_HPP
