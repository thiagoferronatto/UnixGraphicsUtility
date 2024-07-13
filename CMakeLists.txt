cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

project(UnixGraphicsUtility VERSION 0.1 LANGUAGES C CXX)

add_library(GlLoader STATIC src/external/gl.c)

add_executable(u2g src/main.cpp)

include_directories(include)

link_directories(lib)

target_link_libraries(u2g OpenGL GlLoader glfw GLX)