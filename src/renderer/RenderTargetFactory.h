#pragma once

#include "RenderTarget.h"
#include "CPURenderTarget.h"
#include "simd/SimdRenderTarget.h"
#include "embree/EmbreeRenderTarget.h"
// #include "CPUSIMDRenderTarget.h"
// #include "GPURenderTarget.h"  // Uncomment when GPU version is ready

#include <memory>
#include <string>

// Factory for easy render target comparison and benchmarking
class RenderTargetFactory
{
public:
	enum class Type
	{
		CPU,	  // Standard CPU implementation
		CPU_SIMD, // SIMD-optimized CPU (AVX2)
		EMBREE,	  // Embree-accelerated CPU
	};

	static std::unique_ptr<RenderTarget> create(Type type, uint32_t width, uint32_t height)
	{
		switch (type)
		{
		case Type::CPU:
			return std::make_unique<CPURenderTarget>(width, height);

		case Type::CPU_SIMD:
			return std::make_unique<SimdRenderTarget>(width, height);
		case Type::EMBREE:
			return std::make_unique<EmbreeRenderTarget>(width, height);
		default:
			// Fallback to CPU
			return std::make_unique<CPURenderTarget>(width, height);
		}
	}

	static const char *toString(Type type)
	{
		switch (type)
		{
		case Type::CPU:
			return "CPU";
		case Type::CPU_SIMD:
			return "CPU SIMD";
		case Type::EMBREE:
			return "Embree";
		default:
			return "Unknown";
		}
	}
};