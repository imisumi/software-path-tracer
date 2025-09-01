# Simple C++ CMakeLists.txt Template - Explicit Files
cmake_minimum_required(VERSION 3.16)

project(MyApp VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDEs and tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project options
option(USE_PCH "Use precompiled headers" ON)
option(ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Add vendor libraries
add_subdirectory(vendor)

# Find packages BEFORE defining targets
# find_package(fmt REQUIRED)
# find_package(spdlog REQUIRED)

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
    imgui
)
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