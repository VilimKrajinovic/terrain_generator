cmake_minimum_required(VERSION 3.20)
project(terrain_sim VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
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
    src/main.cpp
    # Core
    src/core/app.cpp
    src/core/log.cpp
    # Platform
    src/platform/window.cpp
    src/platform/input.cpp
    # Renderer
    src/renderer/renderer_init.cpp
    src/renderer/renderer_frame.cpp
    src/renderer/renderer_swapchain.cpp
    src/renderer/vk_instance.cpp
    src/renderer/vk_device.cpp
    src/renderer/vk_swapchain.cpp
    src/renderer/vk_sync.cpp
    src/renderer/vk_command.cpp
    src/renderer/vk_shader.cpp
    src/renderer/vk_renderpass.cpp
    src/renderer/vk_pipeline.cpp
    src/renderer/vk_buffer.cpp
    # Geometry
    src/geometry/mesh.cpp
    src/geometry/quad.cpp
    # Simulation
    src/simulation/simulation.cpp
    # Utils
    src/utils/file_io.cpp
    # Memory
    src/memory/arena.cpp
    src/memory/memory.cpp
    # Third party
    third_party/stb_impl.c
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    $<$<CONFIG:Debug>:-g;-O0>
    $<$<NOT:$<CONFIG:Debug>>:-O2>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE
      $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>
  )
endif()

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm/include
    ${Vulkan_INCLUDE_DIRS}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GLM_FORCE_DEPTH_ZERO_TO_ONE
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
