cmake_minimum_required(VERSION 3.25)

project(bot_arena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(bot_arena
    src/main.cpp

    engine/core/Application.cpp
    engine/core/Time.cpp

    engine/platform/sdl/SdlWindow.cpp
    engine/platform/sdl/SdlOpenGLContext.cpp
    
    engine/renderer/opengl/OpenGLRenderer.cpp

    game/BotArenaGame.cpp
)

target_include_directories(bot_arena
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
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