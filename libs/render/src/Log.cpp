#include "render/Log.h"
#include <iostream>

namespace render {

namespace {
    LogCallback g_callback = nullptr;
    LogLevel g_min_level = LogLevel::Info;
}

void Log::set_callback(LogCallback callback) {
    g_callback = std::move(callback);
}

void Log::set_level(LogLevel min_level) {
    g_min_level = min_level;
}

bool Log::should_log(LogLevel level) {
    return static_cast<int>(level) >= static_cast<int>(g_min_level);
}

void Log::send_to_callback(LogLevel level, std::string_view message) {
    if (g_callback) {
        g_callback(level, message);
    } else {
        // Fallback to stdout if no callback is set
        std::cout << message << std::endl;
    }
}

} // namespace render