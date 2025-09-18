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
find_package(OpenImageIO REQUIRED)

# Library source files
file(GLOB_RECURSE RENDER_SOURCES src/*.cpp)

# Create library
add_library(render STATIC ${RENDER_SOURCES})

# Public include directories
target_include_directories(render 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Public dependencies - users get these automatically
target_link_libraries(render 
    PUBLIC
        glm
        embree
	PRIVATE
		OpenImageIO::OpenImageIO
)

# Compiler features
target_compile_features(render PUBLIC cxx_std_23)

# Expose Embree DLL paths for parent projects
if(WIN32)
    set(EMBREE_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin" PARENT_SCOPE)
    set(EMBREE_DLLS
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin/embree4.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin/tbb12.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin/tbbmalloc.dll"
        PARENT_SCOPE
    )
endif()


