#pragma once

#include "RenderTarget.h"
#include "embree/EmbreeRenderTarget.h"

#include <memory>
#include <string>

// Factory for render target creation - Embree only
class RenderTargetFactory
{
public:
	enum class Type
	{
		EMBREE,	  // Embree-accelerated renderer (default and only option)
	};

	static std::unique_ptr<RenderTarget> create(Type /* type */, uint32_t width, uint32_t height)
	{
		// Only Embree is supported now
		return std::make_unique<EmbreeRenderTarget>(width, height);
	}

	static std::unique_ptr<RenderTarget> create(uint32_t width, uint32_t height)
	{
		// Default to Embree
		return std::make_unique<EmbreeRenderTarget>(width, height);
	}

	static const char *toString(Type type)
	{
		switch (type)
		{
		case Type::EMBREE:
			return "Embree";
		default:
			return "Embree";
		}
	}
};