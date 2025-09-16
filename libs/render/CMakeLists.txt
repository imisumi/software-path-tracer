# Render Library - Self-contained
cmake_minimum_required(VERSION 3.16)

# Can be built standalone or as part of larger project
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(render VERSION 1.0.0)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    message(STATUS "Building render library as standalone")
    set(RENDER_STANDALONE ON)
else()
    message(STATUS "Building render library as part of larger project")
    set(RENDER_STANDALONE OFF)
endif()

# Add vendor dependencies (self-contained)
add_subdirectory(vendor/glm)

# Platform-specific Embree paths (relative to render lib)
if(WIN32)
    set(embree_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/lib/cmake/embree-4.4.0")
elseif(APPLE)
    set(embree_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/mac/lib/cmake/embree-4.4.0")
elseif(UNIX)
    set(embree_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/linux/lib/cmake/embree-4.4.0")
endif()

find_package(embree 4 REQUIRED)

# Library source files (when we add .cpp files)
# file(GLOB_RECURSE RENDER_SOURCES src/*.cpp)

# Create library (INTERFACE for now since header-only)
add_library(render INTERFACE)

# Public include directories
target_include_directories(render INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Public dependencies - users get these automatically
target_link_libraries(render INTERFACE
    glm
    embree
)

# Compiler features
target_compile_features(render INTERFACE cxx_std_23)

# When we become a real library with .cpp files, change to:
# add_library(render STATIC ${RENDER_SOURCES})
# target_link_libraries(render PUBLIC glm embree)  # PUBLIC instead of INTERFACE