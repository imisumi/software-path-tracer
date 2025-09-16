#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace render {

#if 0 // Placeholder - shared math utilities
    /// Shared mathematical utilities for rendering
    /// Contains common math functions used across different rendering techniques
    namespace Math {

        // Constants
        constexpr float PI = 3.14159265359f;
        constexpr float TWO_PI = 6.28318530718f;
        constexpr float INV_PI = 0.31830988618f;
        constexpr float EPSILON = 1e-6f;

        // Vector utilities
        glm::vec3 reflect(const glm::vec3& incident, const glm::vec3& normal);
        glm::vec3 refract(const glm::vec3& incident, const glm::vec3& normal, float eta);
        glm::vec3 faceForward(const glm::vec3& n, const glm::vec3& i);

        // Coordinate system construction
        void buildOrthonormalBasis(const glm::vec3& n, glm::vec3& tangent, glm::vec3& bitangent);
        glm::mat3 tangentToWorld(const glm::vec3& normal);

        // Color space conversions
        glm::vec3 linearToSRGB(const glm::vec3& linear);
        glm::vec3 sRGBToLinear(const glm::vec3& srgb);

        // Tonemapping operators
        glm::vec3 acesTonemap(const glm::vec3& hdr, float exposure = 1.0f);
        glm::vec3 reinhardTonemap(const glm::vec3& hdr, float exposure = 1.0f);

        // Utility functions
        float luminance(const glm::vec3& color);
        float schlickFresnel(float cosTheta, float f0);
        bool solveQuadratic(float a, float b, float c, float& t0, float& t1);
    }
#endif

}