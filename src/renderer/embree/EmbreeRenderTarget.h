#pragma once

#include "../RenderTarget.h"
#include "../Texture2D.h"
#include "../HitInfo.h"
#include <memory>
#include <vector>
#include <cstdint>

#include "../Ray.h"

class EmbreeRenderTarget : public RenderTarget {
public:
    EmbreeRenderTarget(uint32_t width, uint32_t height);
    ~EmbreeRenderTarget() = default;

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
    void raygen_shader_single(const Scene& scene, uint32_t x, uint32_t y, uint32_t frame) const;
    void raygen_shader_packet4(const Scene& scene, uint32_t start_x, uint32_t start_y, uint32_t frame) const;
    void raygen_shader_packet8(const Scene& scene, uint32_t start_x, uint32_t start_y, uint32_t frame) const;
    void raygen_shader_packet16(const Scene& scene, uint32_t start_x, uint32_t start_y, uint32_t frame) const;
    
    // Unified packet function (replaces the above 4 functions)
    void raygen_shader_packet(const Scene& scene, uint32_t start_x, uint32_t start_y, uint32_t frame, uint32_t packet_size) const;
    
    // Path tracing
    glm::vec4 trace_ray(const Scene& scene, const glm::vec3& ray_origin, const glm::vec3& ray_direction, uint32_t& rng_state) const;
    glm::vec4 trace_ray_single_bounce(const Scene& scene, const glm::vec3& ray_origin, const glm::vec3& ray_direction, uint32_t& rng_state) const;

    uint32_t colorToRGBA(const glm::vec3& color) const;
    
    // Tonemapping functions
    glm::vec3 aces_tonemap(const glm::vec3& hdr_color, float exposure = 1.0f) const;
    glm::vec3 linear_to_srgb(const glm::vec3& linear_color) const;

	glm::vec3 sample_sky(const glm::vec3& direction, const Scene& scene) const;

    std::unique_ptr<Texture2D> m_texture;
    std::vector<glm::vec4> m_floatData;
    std::vector<uint32_t> m_displayData;
    uint32_t m_frameCount = 0;
    
public:
    // Tonemapping controls
    float m_exposure = 1.0f;
    bool m_auto_exposure = false;
    float m_target_luminance = 0.18f; // Middle gray target
    
    // Auto exposure calculation (public for UI access)
    float calculate_auto_exposure() const;
};