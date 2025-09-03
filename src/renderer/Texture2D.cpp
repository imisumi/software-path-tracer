#include "Texture2D.h"

#include <SDL3/SDL.h>
#include "GraphicsContext.h"
#include <stdexcept>

SDL_PixelFormat convertFormat(Texture2D::Format format)
{
	switch (format)
	{
	case Texture2D::Format::RGBA8:
		return SDL_PIXELFORMAT_RGBA8888;
	default:
		return SDL_PIXELFORMAT_UNKNOWN;
	}
}

Texture2D::Texture2D(uint32_t width, uint32_t height, Format format)
{
	m_texture = SDL_CreateTexture(GraphicsContext::getSDLRenderer(), convertFormat(format), SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!m_texture)
	{
		return;
		throw std::runtime_error("Failed to create SDL texture");
	}

	m_width = width;
	m_height = height;
	m_format = format;
}

Texture2D::~Texture2D()
{
	if (m_texture)
	{
		SDL_DestroyTexture(m_texture);
		m_texture = nullptr;
	}
}

void Texture2D::resize(uint32_t width, uint32_t height)
{
	if (width == m_width && height == m_height)
		return;

	if (m_texture)
	{
		SDL_DestroyTexture(m_texture);
		m_texture = nullptr;
	}
	m_texture = SDL_CreateTexture(GraphicsContext::getSDLRenderer(), convertFormat(m_format), SDL_TEXTUREACCESS_STREAMING, width, height);
	if (!m_texture)
	{
		return;
		throw std::runtime_error("Failed to create SDL texture");
	}
	m_width = width;
	m_height = height;
}

void Texture2D::set_data(const std::vector<uint32_t> &data)
{
	if (data.size() > m_width * m_height)
		throw std::runtime_error("Data size does not match texture size");

	// Fallback to SDL_UpdateTexture if locking fails
	SDL_UpdateTexture(m_texture, nullptr, data.data(), m_width * 4);
}
