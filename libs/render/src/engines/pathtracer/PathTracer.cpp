#include "render/PathTracer.h"

#include "engines/pathtracer/backends/cpu/CPUPathTracer.h"

#include <stdexcept>

namespace render
{
	std::unique_ptr<PathTracer> PathTracer::create_path_tracer(BackendType backend)
	{
		switch (backend)
		{
		case BackendType::CPU_EMBREE:
			return std::make_unique<render::CPUPathTracer>();
		// case BackendType::GPU_OPTIX:
		// 	return PathTracingEngine::create(BackendType::GPU_OPTIX);
		// case BackendType::GPU_METAL:
		// 	return PathTracingEngine::create(BackendType::GPU_METAL);
		default:
			throw std::runtime_error("Unknown backend type");
		}
	}
}