#include "render/Types.h"

namespace render {

    void RenderSettings::setResolution(uint32_t width, uint32_t height) {
        if (m_width != width || m_height != height) {
            m_width = width;
            m_height = height;
            markDirty();
        }
    }

    void RenderSettings::setProgressive(bool progressive) {
        if (m_progressive != progressive) {
            m_progressive = progressive;
            markDirty();
        }
    }

    void RenderSettings::setSamplesPerPixel(uint32_t samples) {
        if (m_samplesPerPixel != samples) {
            m_samplesPerPixel = samples;
            markDirty();
        }
    }

    void RenderSettings::setMaxBounces(uint32_t bounces) {
        if (m_maxBounces != bounces) {
            m_maxBounces = bounces;
            markDirty();
        }
    }

    void RenderSettings::setRussianRouletteDepth(uint32_t depth) {
        if (m_russianRouletteDepth != depth) {
            m_russianRouletteDepth = depth;
            markDirty();
        }
    }

    void RenderSettings::setExposure(float exposure) {
        if (m_exposure != exposure) {
            m_exposure = exposure;
            markDirty();
        }
    }

    void RenderSettings::setAutoExposure(bool enabled, float target_luminance) {
        if (m_autoExposure != enabled || m_targetLuminance != target_luminance) {
            m_autoExposure = enabled;
            m_targetLuminance = target_luminance;
            markDirty();
        }
    }

}