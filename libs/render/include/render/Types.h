#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>
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

    /// Simple render request for legacy compatibility
    struct RenderRequest {
        uint32_t width = 512;
        uint32_t height = 512;
        uint32_t samples_per_pixel = 64;
        uint32_t max_bounces = 8;
        bool progressive = true;
    };

    /// Render settings with automatic dirty flag management
    class RenderSettings {
    public:
		RenderSettings() = default;

        // Image parameters
        void setResolution(uint32_t width, uint32_t height);
        void setProgressive(bool progressive);
        
        // Path tracing settings
        void setSamplesPerPixel(uint32_t samples);
        void setMaxBounces(uint32_t bounces);
        void setRussianRouletteDepth(uint32_t depth);
        
        // Exposure and tone mapping
        void setExposure(float exposure);
        void setAutoExposure(bool enabled, float target_luminance = 0.18f);
        
        // Getters
        uint32_t getWidth() const { return m_width; }
        uint32_t getHeight() const { return m_height; }
        bool getProgressive() const { return m_progressive; }
        uint32_t getSamplesPerPixel() const { return m_samplesPerPixel; }
        uint32_t getMaxBounces() const { return m_maxBounces; }
        uint32_t getRussianRouletteDepth() const { return m_russianRouletteDepth; }
        float getExposure() const { return m_exposure; }
        bool getAutoExposure() const { return m_autoExposure; }
        float getTargetLuminance() const { return m_targetLuminance; }
        
        // Dirty state management
        bool isDirty() const { return m_dirty; }
        void clearDirty() { m_dirty = false; }

    private:
        // Image parameters
        uint32_t m_width = 512;
        uint32_t m_height = 512;
        bool m_progressive = true;
        
        // Path tracing settings
        uint32_t m_samplesPerPixel = 64;
        uint32_t m_maxBounces = 8;
        uint32_t m_russianRouletteDepth = 3;
        
        // Exposure and tone mapping
        float m_exposure = 1.0f;
        bool m_autoExposure = false;
        float m_targetLuminance = 0.18f;
        
        // Dirty flag
        bool m_dirty = true;  // Dirty on construction
        
        void markDirty() { m_dirty = true; }
    };

}