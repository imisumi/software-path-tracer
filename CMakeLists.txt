# Simple C++ CMakeLists.txt Template - Explicit Files
cmake_minimum_required(VERSION 3.16)


# C:\Users\ichir\Documents\vcpkg
# set(CMAKE_TOOLCHAIN_FILE "C:\\Users/ichir/Documents/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Check for toolchain file - use env variable or error
# if(NOT CMAKE_TOOLCHAIN_FILE)
#     if(DEFINED ENV{CMAKE_TOOLCHAIN_FILE})
#         set(CMAKE_TOOLCHAIN_FILE "$ENV{CMAKE_TOOLCHAIN_FILE}" CACHE STRING "")
#         message(STATUS "Using toolchain file from environment: ${CMAKE_TOOLCHAIN_FILE}")
#     else()
#         message(FATAL_ERROR "CMAKE_TOOLCHAIN_FILE environment variable is not set. Please set it to point to your vcpkg toolchain file.")
#     endif()
# endif()

project(MyApp VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project options
option(USE_PCH "Use precompiled headers" ON)
option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)
option(ENABLE_OPENMP "Enable OpenMP multithreading" OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find packages BEFORE defining targets
# find_package(fmt REQUIRED)
# find_package(spdlog REQUIRED)

find_package(SDL3 CONFIG REQUIRED)
find_package(hwy CONFIG REQUIRED)
find_package(OpenImageIO CONFIG REQUIRED)
# embree and glm now provided by pathtracer library

# OpenMP - conditional
if(ENABLE_OPENMP)
    find_package(OpenMP REQUIRED)
    message(STATUS "OpenMP enabled - using multithreading")
else()
    message(STATUS "OpenMP disabled - single-threaded execution")
endif()

# Add vendor libraries (editor-only dependencies)
add_subdirectory(vendor)
# add_subdirectory(vendor/sdl)
# glm now in pathtracer/vendor

# Add render library (self-contained)
add_subdirectory(libs/render)

# Recursively find all source files in src/
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

# Recursively find all header files in src/ (for IDE support)
file(GLOB_RECURSE HEADERS
    src/*.h
)

# Define targets AFTER finding packages
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# clang-tidy integration (optional)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
        )
        message(STATUS "clang-tidy enabled for ${PROJECT_NAME}")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# clang-tidy integration (optional)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
    if(CLANG_TIDY_EXE)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*,-fuchsia-*,-google-*,-zircon-*,-abseil-*,-modernize-use-trailing-return-type"
        )
        message(STATUS "clang-tidy enabled")
    else()
        message(WARNING "clang-tidy requested but not found")
    endif()
endif()

# Configure target properties AFTER defining target
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Precompiled headers (if enabled and pch.h exists)
if(USE_PCH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h)
    target_precompile_headers(${PROJECT_NAME} PRIVATE src/pch.h)
    message(STATUS "Using precompiled headers: src/pch.h")
elseif(USE_PCH)
    message(STATUS "PCH enabled but src/pch.h not found - skipping PCH")
endif()

# Link libraries AFTER target configuration
target_link_libraries(${PROJECT_NAME} PRIVATE
	SDL3::SDL3
    imgui
	hwy::hwy
	OpenImageIO::OpenImageIO
	render  # Provides glm and embree automatically
)

# Link OpenMP conditionally
if(ENABLE_OPENMP)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE OPENMP_ENABLED)
endif()

# Copy SDL3 DLL to output directory on Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying SDL3 DLL to output directory"
    )
    
    # Copy Embree DLLs (provided by render library)
    if(DEFINED EMBREE_DLLS)
        foreach(DLL ${EMBREE_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
                COMMENT "Copying Embree DLL: ${DLL}"
            )
        endforeach()
    endif()
    
    # Copy assets folder to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
        COMMENT "Copying assets folder to output directory"
    )
endif()
# target_link_libraries(${PROJECT_NAME} PRIVATE
#     fmt::fmt
#     spdlog::spdlog
# )

# Determine OS and architecture strings
if(WIN32)
    set(OS_NAME "windows")
elseif(APPLE)
    set(OS_NAME "macos")
elseif(UNIX)
    set(OS_NAME "linux")
else()
    set(OS_NAME "unknown")
endif()

# Determine architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)")
        set(ARCH_NAME "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)")
        set(ARCH_NAME "x64")
    else()
        set(ARCH_NAME "64bit")
    endif()
else()
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm|armv7)")
        set(ARCH_NAME "arm32")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i386|i686|x86)")
        set(ARCH_NAME "x86")
    else()
        set(ARCH_NAME "32bit")
    endif()
endif()

# Set output directory: build/bin/build-type/os/arch/
set_target_properties(${PROJECT_NAME} PROPERTIES
    # RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${OS_NAME}/${ARCH_NAME}
    # RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin/$<CONFIG>/${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}
    # Set working directory for IDEs (Visual Studio, etc.)
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Custom target to run the executable from project root
add_custom_target(run
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running ${PROJECT_NAME} from project root"
)