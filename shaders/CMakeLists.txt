# Shader compilation

# Find glslc compiler
find_program(GLSLC glslc HINTS
    $ENV{VULKAN_SDK}/bin
    /usr/local/bin
    /usr/bin
)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found! Please install Vulkan SDK or set VULKAN_SDK environment variable.")
endif()

message(STATUS "Found glslc: ${GLSLC}")

# Shader source files
set(SHADER_SOURCES
    basic.vert
    basic.frag
)

# Output directory for compiled shaders
set(SHADER_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})

# Compile shaders
set(SHADER_OUTPUTS)
foreach(SHADER ${SHADER_SOURCES})
    set(INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER})
    set(OUTPUT_FILE ${SHADER_OUTPUT_DIR}/${SHADER}.spv)

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${GLSLC} -o ${OUTPUT_FILE} ${INPUT_FILE}
        DEPENDS ${INPUT_FILE}
        COMMENT "Compiling ${SHADER} to SPIR-V"
        VERBATIM
    )

    list(APPEND SHADER_OUTPUTS ${OUTPUT_FILE})
endforeach()

# Create custom target for all shaders
add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
