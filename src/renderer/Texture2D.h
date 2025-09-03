#pragma once

#include <vector>
#include <cstdint>

struct SDL_Texture;

class Texture2D
{
public:
	enum class Format
	{
		RGBA8
	};

public:
	Texture2D(uint32_t width, uint32_t height, Format format);
	~Texture2D();

	void *get_texture() const { return m_texture; }

	void resize(uint32_t width, uint32_t height);

	void set_data(const std::vector<uint32_t> &data);

	uint32_t get_width() const { return m_width; }
	uint32_t get_height() const { return m_height; }

private:
	SDL_Texture *m_texture = nullptr;

	Format m_format = Format::RGBA8;
	uint32_t m_width = 0, m_height = 0;
};