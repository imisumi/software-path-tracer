#include "renderer.h"

#include "Texture2D.h"
#include <thread>
#include <algorithm>
#include <execution>
#include <numeric>

#include <glm/glm.hpp>

#include <SDL3/SDL.h>

void Renderer::on_resize(uint32_t width, uint32_t height)
{
	if (m_texture)
	{
		if (width == m_texture->get_width() && height == m_texture->get_height())
			return;
		m_texture->resize(width, height);
	}
	else
	{
		m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA8);
		m_data.resize(width * height, 0);
	}

	m_data.resize(width * height, 0);
}

// void Renderer::render()
// {
// 	const uint32_t width = m_texture->get_width();
// 	const uint32_t height = m_texture->get_height();
// 	const uint32_t total_pixels = width * height;

// 	// Pre-calculate constants
// 	const float inv_width = 1.0f / width;
// 	const float inv_height = 1.0f / height;
// 	const uint32_t blue_component = (uint32_t)(0.2f * 255) << 8;
// 	const uint32_t alpha_component = 255;

// 	// Parallel processing using std::execution
// 	std::vector<uint32_t> indices(total_pixels);
// 	std::iota(indices.begin(), indices.end(), 0);

// 	std::for_each(std::execution::par_unseq, indices.begin(), indices.end(),
// 		[&](uint32_t i) {
// 			const uint32_t x = i % width;
// 			const uint32_t y = i / width;

// 			const uint32_t r = (uint32_t)(x * inv_width * 255) << 24;
// 			const uint32_t g = (uint32_t)(y * inv_height * 255) << 16;

// 			m_data[i] = r | g | blue_component | alpha_component;
// 		});

// 	m_texture->set_data(m_data);
// }

// void Renderer::render()
// {
// 	const uint32_t width = m_texture->get_width();
// 	const uint32_t height = m_texture->get_height();

// 	// Pre-calculate constants outside loops
// 	const float inv_width = 1.0f / width;
// 	const float inv_height = 1.0f / height;
// 	const uint32_t blue_alpha = ((uint32_t)(0.2f * 255) << 8) | 255; // b and a combined

// 	uint32_t* data_ptr = m_data.data(); // Cache pointer to avoid bounds checking

// 	for (uint32_t y = 0; y < height; ++y)
// 	{
// 		const uint32_t g_component = (uint32_t)(y * inv_height * 255) << 16; // Calculate g once per row

// 		for (uint32_t x = 0; x < width; ++x)
// 		{
// 			const uint32_t r_component = (uint32_t)(x * inv_width * 255) << 24;
// 			*data_ptr++ = r_component | g_component | blue_alpha;
// 		}
// 	}
// 	m_texture->set_data(m_data);
// }

// void Renderer::render()
// {
// 	const uint32_t width = m_texture->get_width();
// 	const uint32_t height = m_texture->get_height();

// 	// Pre-calculate constants outside loops
// 	const float inv_width = 1.0f / width;
// 	const float inv_height = 1.0f / height;
// 	const uint32_t blue_alpha = ((uint32_t)(0.2f * 255) << 8) | 255; // b and a combined

// 	uint32_t* data_ptr = m_data.data(); // Cache pointer to avoid bounds checking

// 	for (uint32_t y = 0; y < height; ++y)
// 	{
// 		const uint32_t g_component = (uint32_t)(y * inv_height * 255) << 16; // Calculate g once per row

// 		for (uint32_t x = 0; x < width; ++x)
// 		{
// 			const uint32_t r_component = (uint32_t)(x * inv_width * 255) << 24;
// 			*data_ptr++ = r_component | g_component | blue_alpha;
// 		}
// 	}
// 	m_texture->set_data(m_data);
// }

float intersect_sphere(const glm::vec3 &O, const glm::vec3 &D, const glm::vec3 &C, float r)
{
	//  |P - C|² = r²
	float t = -1.0f;
	return t;
}

void Renderer::render()
{
	const uint32_t width = m_texture->get_width();
	const uint32_t height = m_texture->get_height();
	for (uint32_t y = 0; y < height; ++y)
	{
		float v = (float)y / height;
		for (uint32_t x = 0; x < width; ++x)
		{
			glm::vec2 pixel_coord = glm::vec2((float)x / width, v) * 2.0f - 1.0f;
			m_data[y * width + x] = per_pixel(pixel_coord);
		}
	}
	m_texture->set_data(m_data);
}

uint32_t Renderer::per_pixel(glm::vec2 uv) const
{
    const glm::vec3 O(0.0f, 0.0f, 0.0f);  // Ray origin
    const glm::vec3 D(uv.x, uv.y, -1.0f); // Ray direction (can be unnormalized)
    const glm::vec3 C(0.0f, 0.0f, -5.0f); // Sphere center
    const float r = 1.0f;                  // Sphere radius

    // Algebraic method from README
    glm::vec3 L = O - C;                   // Vector from sphere center to ray origin
    float a = glm::dot(D, D);              // |D|²
    float b = 2.0f * glm::dot(L, D);       // 2L·D
    float c = glm::dot(L, L) - r * r;      // |L|² - r²
    
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
        return 0xff0000ff;                 // No intersection
        
    float sqrt_discriminant = sqrt(discriminant);
    float t0 = (-b - sqrt_discriminant) / (2 * a);
    float t1 = (-b + sqrt_discriminant) / (2 * a);
    
    // Choose nearest positive intersection
    float t = (t0 > 0) ? t0 : t1;
    if (t < 0)
        return 0xff0000ff;
        
    glm::vec3 P = O + t * D;
    glm::vec3 N = glm::normalize(P - C);
    
    const uint8_t r_col = (uint8_t)((N.x + 1.0f) * 0.5f * 255);
    const uint8_t g_col = (uint8_t)((N.y + 1.0f) * 0.5f * 255);
    const uint8_t b_col = (uint8_t)((N.z + 1.0f) * 0.5f * 255);
    const uint8_t a_col = 255;
    
    return (r_col << 24) | (g_col << 16) | (b_col << 8) | (a_col << 0);
}
