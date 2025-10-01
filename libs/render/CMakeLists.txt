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

# Use prebuilt OSL libraries instead of building from source
set(OSL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/osl")
set(OSL_INCLUDE_DIR "${OSL_ROOT}/include")
set(OSL_LIBRARY_DIR "${OSL_ROOT}/lib")
set(OSL_BIN_DIR "${OSL_ROOT}/bin")

# Find OSL libraries
find_library(OSL_COMP_LIBRARY NAMES oslcomp PATHS ${OSL_LIBRARY_DIR} NO_DEFAULT_PATH)
find_library(OSL_EXEC_LIBRARY NAMES oslexec PATHS ${OSL_LIBRARY_DIR} NO_DEFAULT_PATH)
find_library(OSL_NOISE_LIBRARY NAMES oslnoise PATHS ${OSL_LIBRARY_DIR} NO_DEFAULT_PATH)
find_library(OSL_QUERY_LIBRARY NAMES oslquery PATHS ${OSL_LIBRARY_DIR} NO_DEFAULT_PATH)

# Create imported targets for OSL libraries
add_library(OSL::oslcomp SHARED IMPORTED)
set_target_properties(OSL::oslcomp PROPERTIES
    IMPORTED_LOCATION "${OSL_BIN_DIR}/oslcomp.dll"
    IMPORTED_IMPLIB "${OSL_COMP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OSL_INCLUDE_DIR}"
)

add_library(OSL::oslexec SHARED IMPORTED)
set_target_properties(OSL::oslexec PROPERTIES
    IMPORTED_LOCATION "${OSL_BIN_DIR}/oslexec.dll"
    IMPORTED_IMPLIB "${OSL_EXEC_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OSL_INCLUDE_DIR}"
)

add_library(OSL::oslnoise SHARED IMPORTED)
set_target_properties(OSL::oslnoise PROPERTIES
    IMPORTED_LOCATION "${OSL_BIN_DIR}/oslnoise.dll"
    IMPORTED_IMPLIB "${OSL_NOISE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OSL_INCLUDE_DIR}"
)

add_library(OSL::oslquery SHARED IMPORTED)
set_target_properties(OSL::oslquery PROPERTIES
    IMPORTED_LOCATION "${OSL_BIN_DIR}/oslquery.dll"
    IMPORTED_IMPLIB "${OSL_QUERY_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${OSL_INCLUDE_DIR}"
)

# OSL prebuilt libraries configured above

# Platform-specific Embree paths (relative to render lib)
if(WIN32)
    set(embree_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/lib/cmake/embree-4.4.0")
    
    # Set Flex and Bison executables for Windows
    set(BISON_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/win_flex_bison/win_bison.exe")
    set(FLEX_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/win_flex_bison/win_flex.exe")
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
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>  # For OSL headers
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Public dependencies - users get these automatically
target_link_libraries(render 
    PUBLIC
        glm
        embree
        OSL::oslcomp
        OSL::oslexec
        OSL::oslnoise
        OSL::oslquery
	PRIVATE
		OpenImageIO::OpenImageIO
)

# Compiler features
target_compile_features(render PUBLIC cxx_std_23)

# Expose Embree and OSL DLL paths for parent projects
if(WIN32)
    set(EMBREE_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin" PARENT_SCOPE)
    set(EMBREE_DLLS
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin/embree4.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin/tbb12.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/embree/windows/bin/tbbmalloc.dll"
        PARENT_SCOPE
    )
    
    set(OSL_DLL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/osl/bin" PARENT_SCOPE)
    set(OSL_DLLS
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/osl/bin/oslcomp.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/osl/bin/oslexec.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/osl/bin/oslnoise.dll"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/osl/bin/oslquery.dll"
        PARENT_SCOPE
    )
endif()


