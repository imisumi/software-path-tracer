#pragma once

#include <cstdint>

namespace render
{
	inline uint32_t rgba_to_uint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
	{
		return (r << 24) | (g << 16) | (b << 8) | (a << 0);
	}
} // namespace render
