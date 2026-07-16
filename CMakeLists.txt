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
find_package(freetype CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_library(bot_arena_engine STATIC
    engine/core/Application.cpp
    engine/core/Arena.cpp
    engine/core/Time.cpp
    engine/core/Input.cpp
    engine/core/JobSystem.cpp

    engine/platform/sdl/SdlWindow.cpp
    engine/platform/sdl/SdlOpenGLContext.cpp

    engine/renderer/opengl/OpenGLBackend.cpp
    engine/renderer/opengl/OpenGLBuffer.cpp
    engine/renderer/opengl/OpenGLVertexArray.cpp
    engine/renderer/opengl/OpenGLShader.cpp
    engine/renderer/opengl/OpenGLUniformBuffer.cpp
    engine/renderer/opengl/OpenGLFramebuffer.cpp
    engine/renderer/opengl/OpenGLTextureCube.cpp
    engine/renderer/opengl/OpenGLTexture2D.cpp

    engine/renderer/text/FontManager.cpp
    engine/renderer/text/BitmapFreeTypeSource.cpp
    engine/renderer/text/SdfFreeTypeSource.cpp
    engine/renderer/text/TextRenderer.cpp

    engine/particles/ParticleSystem.cpp

    engine/imgui/ImGuiLayer.cpp

    engine/assets/ModelLoader.cpp
    engine/assets/TextureLoader.cpp

    engine/renderer/RenderBackend.cpp
    engine/renderer/RenderQueue.cpp
    engine/renderer/Renderer.cpp
    engine/renderer/DebugRenderer.cpp
    engine/renderer/ResourceRegistry.cpp
    engine/renderer/MeshRenderer.cpp
    engine/renderer/CommandBufferPool.cpp
)

target_include_directories(bot_arena_engine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Stb_INCLUDE_DIR}
)

target_compile_definitions(bot_arena_engine
    PUBLIC
        BOTARENA_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_link_libraries(bot_arena_engine
    PUBLIC
        SDL3::SDL3
        glad::glad
        glm::glm
        EnTT::EnTT
        spdlog::spdlog
        yaml-cpp::yaml-cpp
        imgui::imgui
        freetype
        assimp::assimp
        Threads::Threads
)

add_executable(bot_arena
    src/main.cpp
    game/BotArenaGame.cpp
)
target_link_libraries(bot_arena PRIVATE bot_arena_engine)

add_executable(arena_game
    games/arena/main.cpp
    games/arena/ArenaGame.cpp
)
target_link_libraries(arena_game PRIVATE bot_arena_engine)

add_executable(particles_game
    games/particles/main.cpp
    games/particles/ParticlesGame.cpp
)
target_link_libraries(particles_game PRIVATE bot_arena_engine)

add_executable(models_game
    games/models/main.cpp
    games/models/ModelsGame.cpp
)
target_link_libraries(models_game PRIVATE bot_arena_engine)

add_executable(shooter_game
    games/shooter/main.cpp
    games/shooter/ShooterGame.cpp
)
target_link_libraries(shooter_game PRIVATE bot_arena_engine)

add_executable(text_demo_game
    games/text_demo/main.cpp
    games/text_demo/TextDemoGame.cpp
)
target_link_libraries(text_demo_game PRIVATE bot_arena_engine)

add_executable(camera_demo_game
    games/camera_demo/main.cpp
    games/camera_demo/CameraDemoGame.cpp
)
target_link_libraries(camera_demo_game PRIVATE bot_arena_engine)

add_executable(billboard_demo_game
    games/billboard_demo/main.cpp
    games/billboard_demo/BillboardDemoGame.cpp
)
target_link_libraries(billboard_demo_game PRIVATE bot_arena_engine)

add_executable(normalmap_demo_game
    games/normalmap_demo/main.cpp
    games/normalmap_demo/NormalMapDemoGame.cpp
)
target_link_libraries(normalmap_demo_game PRIVATE bot_arena_engine)

add_executable(parallax_light_demo_game
    games/parallax_light_demo/main.cpp
    games/parallax_light_demo/ParallaxLightDemoGame.cpp
)
target_link_libraries(parallax_light_demo_game PRIVATE bot_arena_engine)

enable_testing()

add_executable(bot_arena_tests
    tests/test_arena.cpp
    tests/test_sort_key.cpp
    tests/test_render_queue.cpp
    tests/test_resource_registry.cpp
    tests/test_buffer_layout.cpp
    tests/test_shader_parser.cpp
    tests/test_mesh_renderer.cpp
    tests/test_job_system.cpp
    tests/test_command_buffer_pool.cpp
    tests/test_camera_uniforms.cpp
    tests/test_minimap_rect.cpp
    tests/test_light_view_proj.cpp
    tests/test_point_light_ring.cpp
    tests/test_ssao_kernel.cpp
    tests/test_wall_bounce.cpp
    tests/test_fixed_timestep.cpp
    tests/test_agent_collision.cpp
    tests/test_steering.cpp
    tests/test_text_layout.cpp
    tests/test_shelf_packer.cpp
    tests/test_debug_overlay.cpp
    tests/test_combat.cpp
    tests/test_particle.cpp
    tests/test_particle_system.cpp
    tests/test_mesh_bounds.cpp
    tests/test_texture_path.cpp
    tests/test_ship_controls.cpp
    tests/test_font_desc.cpp
    tests/test_text_style.cpp
    tests/test_font_manager.cpp
    tests/test_text_batch.cpp

    engine/core/Arena.cpp
    engine/particles/ParticleSystem.cpp
    engine/core/JobSystem.cpp
    engine/renderer/RenderQueue.cpp
    engine/renderer/ResourceRegistry.cpp
    engine/renderer/MeshRenderer.cpp
    engine/renderer/CommandBufferPool.cpp
    engine/renderer/text/FontManager.cpp
    engine/renderer/text/TextRenderer.cpp
)

target_include_directories(bot_arena_tests
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bot_arena_tests
    PRIVATE glm::glm Catch2::Catch2WithMain Threads::Threads spdlog::spdlog
)

add_test(NAME bot_arena_tests COMMAND bot_arena_tests)