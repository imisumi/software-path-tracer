#pragma once

/// Main include file for the render library
/// Include this in your application to get access to all rendering functionality

// Core interfaces and types
#include "Types.h"
#include "RenderEngine.h"
#include "Scene.h"
#include "Camera.h"

// Implementation note:
// Factory and specific engine implementations are internal to the library
// Applications should use the factory methods to create engines

namespace render {

#if 0 // Forward declarations for public API
    /// Factory function to create render engines - implemented in library
    std::unique_ptr<RenderEngine> createRenderEngine(BackendType backend);
    std::unique_ptr<RenderEngine> createRenderEngine(const std::string& backendName);

    /// Utility functions for backend detection - implemented in library
    std::vector<std::string> getAvailableBackends();
    bool isBackendSupported(BackendType backend);
#endif

}

/// Example usage:
///
/// #include <render/Render.h>
///
/// auto engine = render::createRenderEngine(render::BackendType::CPU_EMBREE);
/// render::RenderRequest request{512, 512, 64, 8, true};
/// auto result = engine->render(request);
///
/// // For progressive rendering:
/// engine->startProgressive(request);
/// while (!engine->isProgressiveReady()) { /* wait */ }
/// auto progressiveResult = engine->getProgressiveResult();