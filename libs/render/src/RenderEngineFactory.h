#pragma once

#include "../include/render/RenderEngine.h"
#include "../include/render/Types.h"
#include <memory>
#include <string>

namespace render {

#if 0 // Placeholder - factory for creating different render engines
    /// Factory class for creating render engines of different types
    /// Provides a unified interface to create CPU and GPU backends
    class RenderEngineFactory {
    public:
        /// Create render engine by backend type
        static std::unique_ptr<RenderEngine> create(BackendType backend);

        /// Create render engine by string name (useful for CLI/config files)
        static std::unique_ptr<RenderEngine> create(const std::string& backendName);

        /// Get list of available backends on current system
        static std::vector<std::string> getAvailableBackends();

        /// Check if specific backend is supported on current system
        static bool isBackendSupported(BackendType backend);

    private:
        // Helper methods
        static std::unique_ptr<RenderEngine> createCPUPathTracer();
        static std::unique_ptr<RenderEngine> createOptixPathTracer();
        static std::unique_ptr<RenderEngine> createMetalPathTracer();
    };
#endif

}