#pragma once

#include <functional>
#include <string_view>
#include <format>

namespace render {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4
};

using LogCallback = std::function<void(LogLevel level, std::string_view message)>;

class Log {
public:
    // Set user callback for log messages
    static void set_callback(LogCallback callback);
    
    // Set minimum log level (messages below this are ignored)
    static void set_level(LogLevel min_level);
    
    // Core logging function
    template<typename... Args>
    static void log(LogLevel level, std::format_string<Args...> fmt, Args&&... args) {
        if (should_log(level)) {
            auto message = std::format(fmt, std::forward<Args>(args)...);
            send_to_callback(level, message);
        }
    }
    
    // Convenience methods
    template<typename... Args>
    static void trace(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Trace, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void debug(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void info(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void warn(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    static void error(std::format_string<Args...> fmt, Args&&... args) {
        log(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

private:
    static bool should_log(LogLevel level);
    static void send_to_callback(LogLevel level, std::string_view message);
};

} // namespace render