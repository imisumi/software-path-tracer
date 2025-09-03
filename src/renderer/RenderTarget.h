#pragma once

#include <glm/glm.hpp>
#include <cstdint>

class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    
    virtual void setPixel(uint32_t x, uint32_t y, const glm::vec3& color) = 0;
    virtual void updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual void clear(const glm::vec3& color = glm::vec3(0.0f)) = 0;
};