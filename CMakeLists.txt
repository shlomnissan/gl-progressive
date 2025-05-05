# Copyright 2024 Betamark Pty Ltd. All rights reserved.
# Author: Shlomi Nissan (shlomi@betamark.com)

cmake_minimum_required(VERSION 3.22.1)

project(gl-progressive)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(${CMAKE_SOURCE_DIR}/cmake)

include(cmake/ShaderString.cmake)
ShaderString()

find_package(OpenGL REQUIRED)
find_package(glad REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

set(CORE_SOURCES
    src/core/geometry.cpp
    src/core/geometry.h
    src/core/orthographic_camera.cpp
    src/core/orthographic_camera.h
    src/core/shaders.cpp
    src/core/shaders.h
    src/core/timer.h
    src/core/window.cpp
    src/core/window.h
    src/geometries/box_geometry.cpp
    src/geometries/box_geometry.h
    src/geometries/plane_geometry.cpp
    src/geometries/plane_geometry.h
)

add_executable(tiling ${LIBS_SOURCES} ${CORE_SOURCES} src/tiling.cpp)

target_include_directories(tiling PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libs
)

target_link_libraries(tiling PRIVATE glfw glad::glad glm::glm OpenGL::GL)