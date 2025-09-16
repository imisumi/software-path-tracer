# Render Library Architecture - Core Design Patterns

## 🎯 Executive Summary

This document outlines the architecture for a **generic rendering library** that supports multiple rendering techniques (path tracing, rasterization, hybrid approaches) with clean separation of concerns between rendering, display, and application logic.

## 🏗️ Core Design Philosophy

### **1. Separation of Concerns**
- **Render Library**: Pure rendering logic, backend-agnostic
- **Display System**: Graphics context management, texture uploading
- **Application**: UI, scene management, user interaction

### **2. Context Ownership Pattern**
```
Application Layer     → Owns Graphics Context (SDL/OpenGL/Metal)
Render Library       → Owns Rendering Contexts (CUDA/OptiX/Metal Compute)
Display Bridge       → Transfers data between contexts
```

### **3. Progressive Rendering Architecture**
- **CPU Rendering**: Returns pixel buffer → Upload to GPU each frame
- **GPU Rendering**: Uses interop for zero-copy texture sharing
- **Backend Agnostic**: Same interface for both approaches

## 📁 Component Responsibilities

### **Render Library (`libs/render/`)**

#### **What it OWNS:**
```cpp
namespace render {
    // ✅ Rendering algorithms and techniques
    class RenderEngine;           // Path tracing, rasterization, etc.
    class Scene;                  // Scene data management
    class Camera;                 // Camera mathematics
    class Material;               // Material definitions

    // ✅ Backend-specific rendering contexts
    RTCDevice embree_device;      // CPU: Embree context
    CUcontext cuda_context;       // GPU: CUDA/OptiX context
    id<MTLDevice> metal_device;   // GPU: Metal compute context

    // ✅ Pure data structures
    struct RenderResult {         // CPU memory only
        std::vector<glm::vec4> pixels;
        uint32_t width, height;
    };
}
```

#### **What it DOES NOT own:**
```cpp
// ❌ NO display/graphics dependencies
SDL_Window* window;              // Application owns this
SDL_Renderer* renderer;          // Application owns this
GLuint opengl_texture;           // Display bridge owns this
ImGui anything;                  // Application UI owns this
```

#### **Interface Contract:**
```cpp
namespace render {
    class RenderEngine {
    public:
        // Core rendering interface
        virtual RenderResult render(const RenderRequest& request) = 0;

        // Progressive rendering interface
        virtual void startProgressive(const RenderRequest& request) = 0;
        virtual bool isProgressiveReady() const = 0;
        virtual const RenderResult& getProgressiveResult() = 0;

        // Optional: GPU interop for zero-copy display
        virtual bool supportsDirectGPUAccess() const { return false; }
        virtual void* getGPUTexture() { return nullptr; }

        // Backend identification
        virtual std::string getBackendName() const = 0;
        virtual BackendType getBackendType() const = 0;
    };
}
```

### **Display Bridge (`apps/editor/src/DisplayBridge.h`)**

#### **Responsibilities:**
```cpp
class DisplayBridge {
public:
    // CPU rendering: Upload pixel buffer to GPU texture
    ImTextureID uploadCPUResult(const render::RenderResult& result);

    // GPU rendering: Setup interop for zero-copy sharing
    void setupGPUInterop(render::RenderEngine* engine);
    ImTextureID getGPUTexture(render::RenderEngine* engine);

private:
    // Owns display-specific GPU resources
    GLuint m_displayTexture;                    // OpenGL
    id<MTLTexture> m_metalTexture;              // Metal
    cudaGraphicsResource_t m_cudaGLResource;    // CUDA↔OpenGL interop
};
```

#### **Data Flow Patterns:**

**CPU Rendering Flow:**
```
1. Render Library → CPU Memory (RenderResult)
2. Display Bridge → Upload to OpenGL texture
3. UI System → Display texture in ImGui
```

**GPU Rendering Flow (Interop):**
```
1. Render Library → GPU Memory (backend-specific)
2. Display Bridge → Setup interop sharing
3. UI System → Display shared texture directly (zero-copy)
```

### **Application Layer (`apps/editor/`, `apps/cli/`)**

#### **Editor Responsibilities:**
```cpp
class EditorApp {
private:
    // ✅ Owns graphics context and window management
    std::unique_ptr<GraphicsContext> m_graphicsContext;
    SDL_Window* m_window;

    // ✅ Owns UI system
    std::unique_ptr<UIManager> m_ui;

    // ✅ Uses render library through interface
    std::unique_ptr<render::RenderEngine> m_renderEngine;

    // ✅ Coordinates between rendering and display
    std::unique_ptr<DisplayBridge> m_displayBridge;

public:
    void renderLoop() {
        // Start progressive rendering
        if (!m_renderEngine->isProgressive()) {
            m_renderEngine->startProgressive(m_renderRequest);
        }

        // Display results
        if (m_renderEngine->isProgressiveReady()) {
            ImTextureID texture;

            if (m_renderEngine->supportsDirectGPUAccess()) {
                // Zero-copy GPU display
                texture = m_displayBridge->getGPUTexture(m_renderEngine.get());
            } else {
                // CPU upload path
                const auto& result = m_renderEngine->getProgressiveResult();
                texture = m_displayBridge->uploadCPUResult(result);
            }

            // Display in UI
            m_ui->displayTexture(texture);
        }
    }
};
```

#### **CLI Responsibilities:**
```cpp
class CLIApp {
    // ✅ Uses render library for batch rendering
    std::unique_ptr<render::RenderEngine> m_renderEngine;

    // ✅ Handles file I/O and command-line interface
    void renderToFile(const std::string& outputPath) {
        auto result = m_renderEngine->render(m_renderRequest);
        saveToFile(result, outputPath);  // No graphics context needed!
    }
};
```

## 🎨 Rendering Technique Scope

### **Should Render Library Support Multiple Techniques?**

**YES** - The render library should support **all rendering techniques**, not just path tracing:

```cpp
namespace render {
    enum class RenderingTechnique {
        PATH_TRACING,        // Monte Carlo path tracing (your current focus)
        RASTERIZATION,       // Traditional GPU rasterization
        RAY_MARCHING,        // SDF/volume rendering
        HYBRID,              // Combination approaches
        NEURAL_RADIANCE,     // NeRF/neural rendering (future)
        PHOTON_MAPPING,      // Photon mapping techniques
    };

    // Different engines for different techniques
    class PathTracingEngine : public RenderEngine {
        // Embree CPU, OptiX GPU, Metal GPU backends
    };

    class RasterizationEngine : public RenderEngine {
        // OpenGL, Vulkan, Metal rasterization backends
    };

    class HybridEngine : public RenderEngine {
        // Real-time rasterization + ray traced reflections/shadows
    };
}
```

### **Why One Library Instead of Multiple?**

#### **✅ Benefits of Unified Library:**
- **Shared Infrastructure**: Scene management, materials, cameras
- **Hybrid Rendering**: Combine techniques (raster + ray tracing)
- **Consistent API**: Same interface for different techniques
- **Code Reuse**: Math, utilities, data structures

#### **✅ Professional Examples:**
- **Blender**: Cycles (path tracing) + Eevee (rasterization) in same render system
- **Unreal**: Lumen combines rasterization + ray tracing + SDF
- **Arnold**: Supports path tracing, photon mapping, and hybrid modes

#### **✅ Future Flexibility:**
```cpp
// Easy to add new techniques later
class MLDenoisingEngine : public RenderEngine {
    // AI/ML-based denoising and upscaling
};

class RealtimePathTracingEngine : public PathTracingEngine {
    // Hardware RT + temporal accumulation + AI denoising
};
```

## 🔄 Interaction Patterns

### **Initialization Pattern:**
```cpp
// 1. Application creates graphics context
auto graphicsContext = GraphicsContext::createOpenGL(1920, 1080, "Editor");

// 2. Application creates display bridge
auto displayBridge = std::make_unique<DisplayBridge>(graphicsContext);

// 3. Application creates render engine
auto renderEngine = RenderEngineFactory::create("embree_path_tracer");

// 4. Setup interop if supported
if (renderEngine->supportsDirectGPUAccess()) {
    displayBridge->setupGPUInterop(renderEngine.get());
}
```

### **Render Loop Pattern:**
```cpp
void update() {
    // 1. Check for render completion
    if (renderEngine->isProgressiveReady()) {

        // 2. Get display texture (backend-specific)
        ImTextureID texture = getDisplayTexture();

        // 3. UI displays texture
        ImGui::Image(texture, size);
    }

    // 4. Continue progressive rendering
    renderEngine->continueProgressive();
}
```

### **Backend Switching Pattern:**
```cpp
void switchRenderingTechnique(RenderingTechnique technique) {
    // Save current state
    auto scene = renderEngine->getScene();
    auto camera = renderEngine->getCamera();

    // Create new engine
    renderEngine = RenderEngineFactory::create(technique);

    // Restore state
    renderEngine->setScene(scene);
    renderEngine->setCamera(camera);

    // Update display bridge
    displayBridge->updateEngine(renderEngine.get());
}
```

## 🚀 Implementation Phases

### **Phase 1: Foundation (Current)**
- ✅ Basic render library with path tracing
- ✅ CPU progressive rendering with buffer upload
- ✅ Clean application/render separation

### **Phase 2: GPU Optimization**
- 🔄 GPU interop for zero-copy display
- 🔄 OptiX backend implementation
- 🔄 Metal backend implementation

### **Phase 3: Multi-Technique Support**
- 📋 Rasterization engine (OpenGL/Vulkan)
- 📋 Hybrid rendering capabilities
- 📋 Real-time preview modes

### **Phase 4: Advanced Features**
- 📋 AI/ML denoising integration
- 📋 Distributed rendering support
- 📋 Advanced material systems

## 🎯 Key Takeaways

1. **Single render library** supporting **multiple rendering techniques**
2. **Clean separation**: Render library never touches display/UI code
3. **Progressive rendering**: CPU uses buffer upload, GPU uses interop
4. **Backend agnostic**: Same interface for CPU/GPU implementations
5. **Future-proof**: Architecture supports adding new techniques easily

This architecture matches how professional renderers (Blender, Arnold, V-Ray) are structured and provides maximum flexibility for future expansion while maintaining clean separation of concerns.