#include "engine/core/Input.hpp"

namespace engine {

std::unordered_map<Key, bool> Input::s_keys;
std::unordered_map<Key, bool> Input::s_previousKeys;
std::unordered_map<MouseButton, bool> Input::s_mouseButtons;

glm::vec2 Input::s_mousePosition{0.0f};
glm::vec2 Input::s_mouseDelta{0.0f};
glm::vec2 Input::s_scrollDelta{0.0f};

void Input::beginFrame() {
  s_previousKeys = s_keys;
  s_mouseDelta = {0.0f, 0.0f};
  s_scrollDelta = {0.0f, 0.0f};
}

void Input::setKey(Key key, bool down) { s_keys[key] = down; }

bool Input::isKeyDown(Key key) { return s_keys[key]; }

bool Input::wasKeyPressed(Key key) {
  return s_keys[key] && !s_previousKeys[key];
}

void Input::setMouseButton(MouseButton button, bool down) {
  s_mouseButtons[button] = down;
}

bool Input::isMouseButtonDown(MouseButton button) {
  return s_mouseButtons[button];
}

void Input::setMousePosition(float x, float y) { s_mousePosition = {x, y}; }

void Input::setMouseDelta(float dx, float dy) { s_mouseDelta = {dx, dy}; }

void Input::setScrollDelta(float dx, float dy) { s_scrollDelta = {dx, dy}; }

glm::vec2 Input::mousePosition() { return s_mousePosition; }

glm::vec2 Input::mouseDelta() { return s_mouseDelta; }

glm::vec2 Input::scrollDelta() { return s_scrollDelta; }

}  // namespace engine
