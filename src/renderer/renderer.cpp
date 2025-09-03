#include "renderer.h"

#include "Texture2D.h"
#include <thread>
#include <algorithm>
#include <execution>
#include <numeric>

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
	const uint8_t r = (uint8_t)((float)uv.x * 255);
	const uint8_t g = (uint8_t)((float)uv.y * 255);
	const uint8_t b = (uint8_t)(0.2f * 255);
	const uint8_t a = 255;





	
	return (r << 24) | (g << 16) | (b << 8) | (a << 0);
}