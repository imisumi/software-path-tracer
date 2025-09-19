#pragma once

#include <cstdio>
#include <cstdlib>
#include <string_view>

#include <source_location>
#include <format>

namespace render {

using source_location = std::source_location;

// Always-active verification for critical invariants
inline void verify(bool condition, 
                  std::string_view message = "Verification failed",
                  const source_location& loc = source_location::current()) {
    if (!condition) {
        auto error_msg = std::format("VERIFY: {} in {}() at {}:{}\n",
                                   message, loc.function_name(), 
                                   loc.file_name(), loc.line());
        std::fputs(error_msg.c_str(), stderr);
        std::abort();
    }
}

// Development-time assertions (disabled in RENDER_RELEASE)
#ifndef RENDER_RELEASE
inline void check(bool condition,
                 std::string_view message = "Check failed", 
                 const source_location& loc = source_location::current()) {
    if (!condition) {
        auto error_msg = std::format("CHECK: {} in {}() at {}:{}\n",
                                   message, loc.function_name(),
                                   loc.file_name(), loc.line());
        std::fputs(error_msg.c_str(), stderr);
        std::abort();
    }
}
#else
inline void check(bool, std::string_view = "", const source_location& = source_location::current()) {
    // No-op in release builds
}
#endif

} // namespace render

// Convenience macros for those who prefer them
// #define VERIFY(condition, ...) render::verify((condition), ##__VA_ARGS__)
// #define CHECK(condition, ...) render::check((condition), ##__VA_ARGS__)

/*
Usage Examples:

#include "runtime_check.h"
using namespace render;

void example() {
    int* ptr = get_pointer();
    
    // Always active - critical for program correctness
    verify(ptr != nullptr, "Null pointer in critical path");
    
    // Active during development (disabled in RENDER_RELEASE)
    check(index < container.size(), "Index out of bounds");
    
    // Or use macros if preferred
    VERIFY(ptr != nullptr, "Null pointer detected");
    CHECK(value > 0, "Expected positive value");
}

Compile-time controls:
- Define RENDER_RELEASE to disable check()
- verify() is always active
*/