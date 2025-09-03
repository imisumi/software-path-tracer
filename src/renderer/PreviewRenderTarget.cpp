// #include "PreviewRenderTarget.h"
// #include <SDL3/SDL.h>
// #include <algorithm>
//
// PreviewRenderTarget::PreviewRenderTarget(SDL_Renderer *sdl_renderer, uint32_t width, uint32_t height)
// 	: m_sdl_renderer(sdl_renderer), m_width(width), m_height(height)
// {
// 	printf("Creating PreviewRenderTarget %dx%d\n", width, height);
//
// 	// Create and own our SDL texture
// 	m_sdl_texture = SDL_CreateTexture(m_sdl_renderer, SDL_PIXELFORMAT_RGBA8888,
// 									  SDL_TEXTUREACCESS_STATIC, width, height);
//
// 	if (!m_sdl_texture)
// 	{
// 		printf("Failed to create SDL texture: %s\n", SDL_GetError());
// 	}
// 	else
// 	{
// 		printf("SDL texture created successfully\n");
// 	}
//
// 	// Create our own buffer
// 	m_buffer.resize(width * height);
// 	printf("Buffer resized to %zu elements\n", m_buffer.size());
// }
//
// void PreviewRenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec3 &color)
// {
// 	if (x >= m_width || y >= m_height)
// 		return;
//
// 	uint32_t index = y * m_width + x;
// 	m_buffer[index] = colorToRGBA(color);
//
// 	// Debug: Print first few pixels
// 	if (x < 3 && y < 3)
// 	{
// 		printf("setPixel(%d, %d) = (%f, %f, %f) -> 0x%08x\n",
// 			   x, y, color.r, color.g, color.b, m_buffer[index]);
// 	}
// }
//
// PreviewRenderTarget::~PreviewRenderTarget()
// {
// 	if (m_sdl_texture)
// 	{
// 		SDL_DestroyTexture(m_sdl_texture);
// 	}
// }
//
// void PreviewRenderTarget::updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
// {
// 	// Update our SDL texture with the changed region
// 	SDL_Rect rect = {(int)x, (int)y, (int)width, (int)height};
// 	SDL_UpdateTexture(m_sdl_texture, &rect, m_buffer.data() + (y * m_width + x), m_width * 4);
// }
//
// void PreviewRenderTarget::clear(const glm::vec3 &color)
// {
// 	uint32_t rgba_color = colorToRGBA(color);
// 	std::fill(m_buffer.begin(), m_buffer.end(), rgba_color);
//
// 	// Update the entire SDL texture
// 	SDL_UpdateTexture(m_sdl_texture, nullptr, m_buffer.data(), m_width * 4);
// }
//
// uint32_t PreviewRenderTarget::colorToRGBA(const glm::vec3 &color)
// {
// 	uint8_t r = (uint8_t)(std::clamp(color.r, 0.0f, 1.0f) * 255.0f);
// 	uint8_t g = (uint8_t)(std::clamp(color.g, 0.0f, 1.0f) * 255.0f);
// 	uint8_t b = (uint8_t)(std::clamp(color.b, 0.0f, 1.0f) * 255.0f);
// 	uint8_t a = 255;
//
// 	return (r << 24) | (g << 16) | (b << 8) | (a << 0);
// }