#pragma once

#include "Types.h"
#include <memory>
#include <string>

namespace render {

    /// Base interface for all rendering engines
    /// Provides common interface for different rendering techniques (path tracing, rasterization, etc.)
    class RenderEngine {
    public:
        virtual ~RenderEngine() = default;

        // Core rendering interface
        virtual RenderResult render(const RenderRequest& request) = 0;

        // Progressive rendering interface for real-time preview
        virtual void startProgressive(const RenderRequest& request) = 0;
        virtual bool isProgressiveReady() const = 0;
        virtual const RenderResult& getProgressiveResult() = 0;

        // Optional: GPU interop for zero-copy display (future optimization)
        virtual bool supportsDirectGPUAccess() const { return false; }
        virtual void* getGPUTexture() { return nullptr; }

        // Backend identification
        virtual std::string getBackendName() const = 0;
        virtual BackendType getBackendType() const = 0;
        virtual RenderingTechnique getRenderingTechnique() const = 0;
    };

}