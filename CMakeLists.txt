cmake_minimum_required(VERSION 3.20)
project(terrain_sim VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Compiler flags
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
  else()
    add_compile_options(-O2)
  endif()
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Find SDL3
find_package(SDL3 REQUIRED CONFIG)

# MoltenVK (macOS)
if(APPLE)
  # MoltenVK is typically included with Vulkan SDK on macOS
  set(CMAKE_MACOSX_RPATH ON)
endif()

# Source files
set(SOURCES
    src/main.c
    # Core
    src/core/app.c
    src/core/log.c
    # Platform
    src/platform/window.c
    src/platform/input.c
    # Renderer
    src/renderer/renderer_init.c
    src/renderer/renderer_frame.c
    src/renderer/renderer_swapchain.c
    src/renderer/vk_instance.c
    src/renderer/vk_device.c
    src/renderer/vk_swapchain.c
    src/renderer/vk_sync.c
    src/renderer/vk_command.c
    src/renderer/vk_shader.c
    src/renderer/vk_renderpass.c
    src/renderer/vk_pipeline.c
    src/renderer/vk_buffer.c
    # Geometry
    src/geometry/mesh.c
    src/geometry/quad.c
    # Simulation
    src/simulation/simulation.c
    # Utils
    src/utils/file_io.c
    # Memory
    src/memory/arena.c
    src/memory/memory.c
    # Third party
    third_party/stb_impl.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
    ${Vulkan_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
)

# macOS specific
if(APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
    )
endif()

# Shader compilation
add_subdirectory(shaders)

# Copy shaders to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/shaders/basic.vert.spv
        ${CMAKE_BINARY_DIR}/shaders/basic.frag.spv
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/
    COMMENT "Copying shaders to output directory"
)

# Make sure shaders are built before the main executable
add_dependencies(${PROJECT_NAME} shaders)

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Vulkan found: ${Vulkan_FOUND}")
message(STATUS "Vulkan include: ${Vulkan_INCLUDE_DIRS}")
message(STATUS "SDL3 found: ${SDL3_FOUND}")
