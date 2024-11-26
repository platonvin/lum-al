cmake_minimum_required(VERSION 3.21)
project(lumal LANGUAGES C CXX)

# Some settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Source files for lumal
file(GLOB_RECURSE LUMAL_SOURCES
    src/*.cpp
    src/*.c
    external/volk/volk.c
)
set(LUMAL_SOURCES ${LUMAL_SOURCES} CACHE INTERNAL "Lum-al sources")

# glfw - library for windowing
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw)
    add_subdirectory(external/glfw)
else()
    message(FATAL_ERROR "GLFW directory not found.")
endif()

# glslang - glsl to spirv compiler
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/glsl)
    set(ENABLE_GLSLANG_BINARIES ON CACHE BOOL "Enable building GLSL binaries")
    add_subdirectory(external/glsl ${CMAKE_BINARY_DIR}/glslang_build)
    # set_target_properties(glslangValidator PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
    message(FATAL_ERROR "GLSL directory not found.")
endif()

# Create lum-al library
add_library(lumal ${LUMAL_SOURCES})

# Include directories for lumal (used by downstream projects)
target_include_directories(lumal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src           # Just sources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/defines   # Internal macros
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glm  # For <glm/glm.hpp>
    ${CMAKE_CURRENT_SOURCE_DIR}/external      # For volk/vma/vulkan
    ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include # GLFW
)
    
# Allow downstream projects to specify glfw (and other dependencies) externally
target_link_libraries(lumal PUBLIC glfw)

# Install ?

# Optional hello triangle example (can be disabled)
option(LUM_AL_BUILD_EXAMPLES "Build lumal examples" ON)

if(LUM_AL_BUILD_EXAMPLES)
    add_executable(triangle_example examples/triangle.cpp)
    target_link_libraries(triangle_example PRIVATE lumal)
    target_include_directories(triangle_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(triangle_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
    )
endif()