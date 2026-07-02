#ifndef ENGINE_IMGUI_IMGUILAYER_HPP
#define ENGINE_IMGUI_IMGUILAYER_HPP

#include "engine/core/Layer.hpp"

namespace engine {

// Hazel-style ImGui layer: owns the ImGui context and per-frame begin/end.
class ImGuiLayer : public Layer {
 public:
  ImGuiLayer(void* sdlWindow, void* glContext);

  void onAttach() override;
  void onDetach() override;

  void begin();  // start a new ImGui frame
  void end();    // render ImGui draw data to the bound framebuffer

 private:
  void* m_window = nullptr;
  void* m_glContext = nullptr;
};

}  // namespace engine

#endif  // ENGINE_IMGUI_IMGUILAYER_HPP
