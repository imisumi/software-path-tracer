#pragma once

#include <glm/glm.hpp>
#include <cstdint>

// Hit information for a single ray-object intersection
// Designed to be SIMD-friendly when used in arrays
struct HitInfo {
    float t = -1.0f;                    // Intersection distance (-1 = no hit)
    uint32_t object_id = UINT32_MAX;    // Which object was hit
    uint32_t primitive_id = 0;          // Which primitive in object (for meshes)
    uint32_t material_id = 0;           // Material index

	glm::vec4 color = glm::vec4(0.0f); // Accumulated color (for debugging)
    
    // Surface properties (calculated on-demand to save memory/bandwidth)
    glm::vec3 position = glm::vec3(0.0f);   // Hit point in world space
    glm::vec3 normal = glm::vec3(0.0f);     // Surface normal
    glm::vec2 uv = glm::vec2(0.0f);         // Texture coordinates (future)
    
    // Convenience methods
    bool is_hit() const { return t > 0.0f; }
    void clear() { t = -1.0f; object_id = UINT32_MAX; }
};

// SIMD-friendly arrays for batch processing
// Used when tracing multiple rays simultaneously
struct HitInfoSoA {
    static constexpr size_t SIMD_WIDTH = 8;  // AVX2 width
    
    float t[SIMD_WIDTH];
    uint32_t object_id[SIMD_WIDTH];
    uint32_t material_id[SIMD_WIDTH];
    
    // Surface data calculated after finding closest hits
    float pos_x[SIMD_WIDTH], pos_y[SIMD_WIDTH], pos_z[SIMD_WIDTH];
    float normal_x[SIMD_WIDTH], normal_y[SIMD_WIDTH], normal_z[SIMD_WIDTH];
    
    void clear() {
        for (int i = 0; i < SIMD_WIDTH; ++i) {
            t[i] = -1.0f;
            object_id[i] = UINT32_MAX;
        }
    }
    
    void set_hit(int lane, float hit_t, uint32_t obj_id, uint32_t mat_id) {
        t[lane] = hit_t;
        object_id[lane] = obj_id;
        material_id[lane] = mat_id;
    }
    
    bool is_hit(int lane) const { return t[lane] > 0.0f; }
};