#pragma once

#include "RenderTarget.h"
#include "Texture2D.h"
#include "HitInfo.h"
#include <memory>
#include <vector>
#include <cstdint>

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

class CPURenderTarget : public RenderTarget {
public:
    CPURenderTarget(uint32_t width, uint32_t height);
    ~CPURenderTarget() = default;

    // Main rendering - implements CPU path tracing
    void render(const Scene& scene, uint32_t frame) override;
    
    // Utility methods
    void setPixel(uint32_t x, uint32_t y, const glm::vec3& color) override;
    void setPixel(uint32_t x, uint32_t y, const glm::vec4& color);
    void updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;
    void clear(const glm::vec3& color = glm::vec3(0.0f)) override;

    void resize(uint32_t width, uint32_t height);
    
    // For ImGui display - exposes the texture
    const std::unique_ptr<Texture2D>& getTexture() const { return m_texture; }
    
    // Direct access to pixel data for batch operations
    std::vector<glm::vec4>& getFloatData() { return m_floatData; }
    const std::vector<glm::vec4>& getFloatData() const { return m_floatData; }
    
    // Commit all pixel changes to the texture (call after rendering)
    void commitPixels();

private:
    // Ray tracing pipeline stages
    glm::vec4 raygen_shader(const Scene& scene, uint32_t x, uint32_t y, uint32_t frame) const;
    glm::vec4 trace_ray(const Scene& scene, const glm::vec3& ray_origin, const glm::vec3& ray_direction, uint32_t& rng_state) const;
    bool anyhit_shader(const glm::vec3& ray_origin, const glm::vec3& ray_direction, HitInfo& hit) const;
    glm::vec4 closesthit_shader(const Scene& scene, const glm::vec3& ray_origin, const glm::vec3& ray_direction, const HitInfo& hit) const;
    glm::vec4 miss_shader(const glm::vec3& ray_direction) const;
    
    // Hit testing - returns closest hit info
    HitInfo intersect_scene(const glm::vec3& ray_origin, const glm::vec3& ray_direction, const Scene& scene) const;
    bool intersect_sphere(const glm::vec3& ray_origin, const glm::vec3& ray_direction,
                         const glm::vec3& sphere_center, float sphere_radius, 
                         uint32_t sphere_id, uint32_t material_id, HitInfo& hit) const;
    
    // Surface calculations
    void calculate_surface_properties(const Scene& scene, const glm::vec3& ray_origin, 
                                     const glm::vec3& ray_direction, HitInfo& hit) const;
    
    uint32_t colorToRGBA(const glm::vec3& color) const;
    
    std::unique_ptr<Texture2D> m_texture;
    std::vector<glm::vec4> m_floatData;
    std::vector<uint32_t> m_displayData;
    uint32_t m_frameCount = 0;
};