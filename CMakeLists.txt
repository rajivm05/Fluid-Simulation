cmake_minimum_required(VERSION 3.14)
project(PositionBasedFluids)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------
# Source files
# --------------------------------------
# file(GLOB SOURCES "src/*.cpp")
file(GLOB_RECURSE SOURCES "src/*.cpp")

# --------------------------------------
# External Libraries
# --------------------------------------

# GLFW (windowing/input)
add_subdirectory(external/glfw)

# GLAD (OpenGL loader)
add_library(glad external/glad/src/gl.c)

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
add_executable(simulator ${SOURCES})

# --------------------------------------
# Link Everything
# --------------------------------------
target_link_libraries(simulator
    glad
    glfw
    ${OPENGL_gl_LIBRARY}
)
