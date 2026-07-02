#include "engine/imgui/ImGuiLayer.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

namespace engine {

ImGuiLayer::ImGuiLayer(void* sdlWindow, void* glContext)
    : m_window(sdlWindow), m_glContext(glContext) {}

void ImGuiLayer::onAttach() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplSDL3_InitForOpenGL(static_cast<SDL_Window*>(m_window), m_glContext);
  ImGui_ImplOpenGL3_Init("#version 460");
}

void ImGuiLayer::onDetach() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::begin() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::end() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

}  // namespace engine
