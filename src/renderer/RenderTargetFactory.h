#pragma once

#include "RenderTarget.h"
#include "CPURenderTarget.h"
// #include "CPUSIMDRenderTarget.h"
// #include "GPURenderTarget.h"  // Uncomment when GPU version is ready

#include <memory>
#include <string>

// Factory for easy render target comparison and benchmarking
class RenderTargetFactory {
public:
    enum class Type {
        CPU,           // Standard CPU implementation
        CPU_SIMD,      // SIMD-optimized CPU (AVX2)
        GPU_OPENGL,    // GPU compute shader (future)
        GPU_CUDA,      // CUDA implementation (future)
        GPU_OPTIX      // OptiX ray tracing (future)
    };

    static std::unique_ptr<RenderTarget> create(Type type, uint32_t width, uint32_t height) {
        switch (type) {
            case Type::CPU:
                return std::make_unique<CPURenderTarget>(width, height);
                
            // case Type::CPU_SIMD:
                // return std::make_unique<CPUSIMDRenderTarget>(width, height);
                
            // case Type::GPU_OPENGL:
            //     return std::make_unique<GPURenderTarget>(width, height);
            
            default:
                // Fallback to CPU
                return std::make_unique<CPURenderTarget>(width, height);
        }
    }
    
    static const char* toString(Type type) {
        switch (type) {
            case Type::CPU: return "CPU";
            case Type::CPU_SIMD: return "CPU SIMD";
            case Type::GPU_OPENGL: return "GPU OpenGL";
            case Type::GPU_CUDA: return "GPU CUDA";
            case Type::GPU_OPTIX: return "GPU OptiX";
            default: return "Unknown";
        }
    }
};