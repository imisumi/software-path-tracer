#pragma once

#include "../../../include/render/RenderEngine.h"
#include "../../../include/render/Types.h"
#include <memory>

namespace render {

#if 0 // Placeholder - to be implemented later
    /// Path tracing specific render engine
    /// Implements Monte Carlo path tracing with different backend support (CPU, GPU)
    class PathTracingEngine : public RenderEngine {
    public:
        /// Factory method to create path tracing backends
        static std::unique_ptr<PathTracingEngine> create(BackendType backend);

        // Path tracing specific settings
        void setPathTracingSettings(const PathTracingSettings& settings);
        const PathTracingSettings& getPathTracingSettings() const;

        // RenderEngine interface
        RenderingTechnique getRenderingTechnique() const override {
            return RenderingTechnique::PATH_TRACING;
        }

    protected:
        PathTracingSettings m_settings;
    };
#endif

}