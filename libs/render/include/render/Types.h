#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace render {

    /// Basic ray structure - works for both CPU and GPU backends
    struct Ray {
        glm::vec3 origin{0.0f};
        glm::vec3 direction{0.0f};

        Ray() = default;
        Ray(const glm::vec3& origin, const glm::vec3& direction)
            : origin(origin), direction(direction) {
        }
    };

    /// Backend types for different rendering implementations
    enum class BackendType {
        CPU_EMBREE,     // CPU ray tracing with Embree acceleration
        GPU_OPTIX,      // NVIDIA OptiX hardware ray tracing
        GPU_METAL       // Apple Metal ray tracing
    };

    /// Different rendering techniques supported
    enum class RenderingTechnique {
        PATH_TRACING,   // Monte Carlo path tracing
        RASTERIZATION,  // Traditional GPU rasterization (future)
        HYBRID          // Combination approaches (future)
    };

    /// Render result containing CPU pixel data
    struct RenderResult {
        std::vector<glm::vec4> pixels;  // RGBA float pixels in CPU memory
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t frame_number = 0;      // For progressive rendering

        /// Direct access to pixel data for GPU upload
        const void* getPixelData() const { return pixels.data(); }
        size_t getDataSize() const { return pixels.size() * sizeof(glm::vec4); }
    };

    /// Parameters for render requests
    struct RenderRequest {
        uint32_t width = 512;
        uint32_t height = 512;
        uint32_t samples_per_pixel = 64;
        uint32_t max_bounces = 8;
        bool progressive = true;
    };

    /// Settings specific to path tracing
    struct PathTracingSettings {
        uint32_t max_bounces = 8;
        uint32_t samples_per_pixel = 64;
        uint32_t russian_roulette_depth = 3;
        float exposure = 1.0f;
        bool auto_exposure = false;
        float target_luminance = 0.18f;
    };

}