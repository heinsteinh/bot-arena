#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>

static void check(bool condition, const std::string& message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

int main() {
  try {
    check(SDL_Init(SDL_INIT_VIDEO), "Failed to initialize SDL3");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window =
        SDL_CreateWindow("Bot Arena - SDL3 OpenGL", 1280, 720,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    check(window != nullptr, "Failed to create SDL window");

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    check(glContext != nullptr, "Failed to create OpenGL context");

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    check(
        gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress)),
        "Failed to load OpenGL via GLAD");

    spdlog::info("OpenGL: {}",
                 reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplSDL3_InitForOpenGL(window, glContext);
    ImGui_ImplOpenGL3_Init("#version 460 core");

    bool running = true;

    while (running) {
      SDL_Event event;

      while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
          running = false;
        }

        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
          running = false;
        }
      }

      int width = 0;
      int height = 0;
      SDL_GetWindowSize(window, &width, &height);

      glViewport(0, 0, width, height);
      glEnable(GL_DEPTH_TEST);

      glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL3_NewFrame();
      ImGui::NewFrame();

      ImGui::Begin("Bot Arena Debug");
      ImGui::Text("Engine bootstrap running");
      ImGui::Text("SDL3 + OpenGL + GLAD + ImGui");
      ImGui::Text("Next: abstract Window and Renderer");
      ImGui::End();

      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
  } catch (const std::exception& e) {
    spdlog::error("Fatal error: {}", e.what());
    SDL_Quit();
    return 1;
  }
}