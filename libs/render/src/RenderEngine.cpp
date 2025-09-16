#include "render/RenderEngine.h"
#include "engines/pathtracer/backends/cpu/CPUPathTracer.h"
#include <stdexcept>

namespace render
{

std::unique_ptr<RenderEngine> RenderEngine::createRenderEngine(BackendType backend)
{
    switch (backend)
    {
    case BackendType::CPU_EMBREE:
        return std::make_unique<CPUPathTracer>();
    case BackendType::GPU_OPTIX:
        throw std::runtime_error("GPU OptiX backend not yet implemented");
    case BackendType::GPU_METAL:
        throw std::runtime_error("GPU Metal backend not yet implemented");
    default:
        throw std::runtime_error("Unknown backend type");
    }
}

}