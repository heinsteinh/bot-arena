cmake_minimum_required(VERSION 3.25)

project(bot_arena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Emit compile_commands.json so clangd / IDEs can resolve includes.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

add_executable(bot_arena
    src/main.cpp

    engine/core/Application.cpp
    engine/core/Arena.cpp
    engine/core/Time.cpp
    engine/core/Input.cpp

    engine/platform/sdl/SdlWindow.cpp
    engine/platform/sdl/SdlOpenGLContext.cpp

    engine/renderer/opengl/OpenGLBackend.cpp
    engine/renderer/opengl/OpenGLBuffer.cpp
    engine/renderer/opengl/OpenGLVertexArray.cpp
    engine/renderer/opengl/OpenGLShader.cpp

    engine/renderer/RenderBackend.cpp
    engine/renderer/RenderQueue.cpp
    engine/renderer/Renderer.cpp
    engine/renderer/DebugRenderer.cpp
    engine/renderer/ResourceRegistry.cpp

    game/BotArenaGame.cpp
)

target_include_directories(bot_arena
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Stb_INCLUDE_DIR}
)

target_compile_definitions(bot_arena
    PRIVATE
        BOTARENA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_link_libraries(bot_arena
    PRIVATE
        SDL3::SDL3
        glad::glad
        glm::glm
        EnTT::EnTT
        spdlog::spdlog
        yaml-cpp::yaml-cpp
        imgui::imgui
)

enable_testing()

add_executable(bot_arena_tests
    tests/test_arena.cpp
    tests/test_sort_key.cpp
    tests/test_render_queue.cpp
    tests/test_resource_registry.cpp
    tests/test_buffer_layout.cpp
    tests/test_shader_parser.cpp

    engine/core/Arena.cpp
    engine/renderer/RenderQueue.cpp
    engine/renderer/ResourceRegistry.cpp
)

target_include_directories(bot_arena_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bot_arena_tests
    PRIVATE glm::glm Catch2::Catch2WithMain
)

add_test(NAME bot_arena_tests COMMAND bot_arena_tests)