cmake_minimum_required(VERSION 3.14)
project(PositionBasedFluids)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_SOURCE_DIR}/CMakeCommands.txt")
    include(${CMAKE_SOURCE_DIR}/CMakeCommands.txt)
endif()

# --------------------------------------
# Source files
# --------------------------------------
file(GLOB_RECURSE SOURCES "src/*.cpp")

# --------------------------------------
# External Libraries
# --------------------------------------

# GLFW (windowing/input)
add_subdirectory(external/glfw)

# GLAD (OpenGL loader)
add_library(glad external/glad/src/gl.c)

add_library(shader src/shader.cpp)
target_link_libraries(shader glad)
target_include_directories(shader PUBLIC src/include)

# --------------------------------------
# Include Directories
# --------------------------------------
include_directories(
    external/glad/include
    external/glfw/include
    external/glm
)

# --------------------------------------
# Find System OpenGL
# --------------------------------------
find_package(OpenGL REQUIRED)

# --------------------------------------
# Executable
# --------------------------------------
add_executable(simulator src/main.cpp)

# --------------------------------------
# Link Everything
# --------------------------------------
target_link_libraries(simulator
    glad
    glfw
    shader
    ${OPENGL_gl_LIBRARY}
)

if(EXISTS "${CMAKE_SOURCE_DIR}/CMakeWindows.txt")
    include(${CMAKE_SOURCE_DIR}/CMakeWindows.txt)
endif()
