cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

set(CMAKE_C_FLAGS_DEBUG "-Og -g -pg")
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native")

project(UnixGraphicsUtility VERSION 0.1 LANGUAGES C CXX)

add_library(GlLoader STATIC src/external/gl.c)

add_executable(u2g
  src/main.cpp
  src/window.cpp
  src/external/libaffa/aa_aafapprox.cpp
  src/external/libaffa/aa_aafarithm.cpp
  src/external/libaffa/aa_aafcommon.cpp
  src/external/libaffa/aa_aaftrigo.cpp
  src/external/libaffa/aa_interval.cpp
  src/external/libaffa/aa_rounding.cpp)

include_directories(include)
include_directories(include/external)

target_link_libraries(u2g OpenGL GlLoader glfw GLX)
