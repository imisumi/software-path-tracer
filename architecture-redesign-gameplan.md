# Path Tracer Architecture Redesign - Game Plan

## 🎯 Executive Summary

This document outlines the transformation of your current monolithic path tracer into a clean, modular architecture that separates the rendering engine from UI concerns, enables multiple backends, and supports both GUI and CLI interfaces.

## 📊 Current Architecture Analysis

### Problems with Current Design
- **Monolithic coupling**: `App.cpp` mixes UI (SDL/ImGui) with rendering logic
- **Backend lock-in**: Scene class is tightly coupled to Embree
- **Context confusion**: Graphics display context mixed with rendering logic
- **No library separation**: Everything built as single executable
- **Limited extensibility**: Hard to add Optix, Metal, or other backends
- **Missing abstractions**: No clean separation between scene data and acceleration structures

### Current Structure
```
MyApp.exe
├── App (UI + Rendering mixed)
├── Scene (Embree-specific)
├── RenderTarget (Display + Rendering mixed)
└── Renderer (Thin static wrapper)
```

## 🏗️ Target Architecture

### High-Level Design Philosophy
1. **Library-First**: Core renderer as reusable library
2. **Backend Agnostic**: Clean abstractions for different ray tracing backends
3. **Context Separation**: Rendering context separate from display context
4. **Modular Components**: Scene, post-processing, and backends as pluggable modules
5. **Interface Flexibility**: Support both GUI and CLI workflows

### New Architecture Overview
```
┌─────────────────────────────────────────────────────────────┐
│                    Applications Layer                       │
├─────────────────────────────────────────────────────────────┤
│  PathTracerEditor  │  PathTracerCLI  │  Future Apps...      │
│  (GUI Application) │  (CLI Tool)     │                      │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  PathTracer Library                         │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐  │
│  │ Scene Manager   │  │ Post Processing │  │ Image I/O   │  │
│  │                 │  │                 │  │             │  │
│  └─────────────────┘  └─────────────────┘  └─────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                  Rendering Engine Core                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐  │
│  │ Renderer        │  │ Camera System   │  │ Material    │  │
│  │ Interface       │  │                 │  │ System      │  │
│  └─────────────────┘  └─────────────────┘  └─────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    Backend Abstraction                      │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐  │
│  │ CPU RayTracing  │  │ Optix RayTracing│  │ Metal       │  │
│  │ Backend         │  │ Backend         │  │ RayTracing  │  │
│  │ (Embree)        │  │ (NVIDIA RTX)    │  │ Backend     │  │
│  └─────────────────┘  └─────────────────┘  └─────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 📁 Detailed Component Design

### 1. PathTracer Library Core (`libpathtracer`)

#### 1.1 Rendering Engine Interface
```cpp
// Core rendering interface - backend agnostic
class RenderEngine {
public:
    virtual ~RenderEngine() = default;
    
    // Configuration
    virtual void setScene(std::shared_ptr<Scene> scene) = 0;
    virtual void setCamera(const Camera& camera) = 0;
    virtual void setRenderSettings(const RenderSettings& settings) = 0;
    
    // Rendering operations
    virtual RenderResult render(const RenderRequest& request) = 0;
    virtual void renderProgressive(const RenderRequest& request, 
                                 ProgressCallback callback) = 0;
    
    // Capabilities query
    virtual RenderBackendCapabilities getCapabilities() const = 0;
    virtual std::string getBackendName() const = 0;
};
```

#### 1.2 Scene Management (Backend Independent)
```cpp
// Pure scene data - no backend coupling
class Scene {
public:
    // Geometry management
    void addGeometry(std::shared_ptr<Geometry> geometry);
    void removeGeometry(GeometryID id);
    void updateGeometry(GeometryID id, const Transform& transform);
    
    // Environment & lighting
    void setEnvironmentMap(std::shared_ptr<EnvironmentMap> envMap);
    void addLight(std::shared_ptr<Light> light);
    
    // Material management
    MaterialID addMaterial(std::shared_ptr<Material> material);
    void updateMaterial(MaterialID id, std::shared_ptr<Material> material);
    
    // Change tracking for backends
    const SceneChangeSet& getChanges() const;
    void markChangesProcessed();
    
private:
    SceneGraph m_sceneGraph;
    MaterialLibrary m_materials;
    LightingEnvironment m_lighting;
    SceneChangeSet m_changes;
};
```

#### 1.3 Graphics System Architecture - Clean Separation

**Key Insight**: Both UI and renderer need the same graphics context for texture sharing!

```cpp
// Pure graphics API abstraction - no window management
class GraphicsAPI {
public:
    virtual ~GraphicsAPI() = default;
    
    // Core API functions
    virtual void* getNativeContext() = 0;
    virtual GraphicsAPIType getType() const = 0; // OpenGL, Metal, Vulkan
    virtual bool isValid() const = 0;
    
    // Resource creation
    virtual std::unique_ptr<Texture> createTexture(uint32_t width, uint32_t height, 
                                                   PixelFormat format) = 0;
    virtual std::unique_ptr<ComputeShader> createComputeShader(const std::string& source) = 0;
    
    // Compute operations
    virtual bool supportsCompute() const = 0;
    virtual void dispatchCompute(ComputeShader& shader, uint32_t x, uint32_t y, uint32_t z) = 0;
    
    // Error handling
    virtual std::string getLastError() const = 0;
    virtual void clearErrors() = 0;
};

// Window management - separate responsibility 
class Window {
public:
    virtual ~Window() = default;
    
    // Window operations
    virtual void* getNativeWindow() = 0;
    virtual void swapBuffers() = 0;
    virtual void setVSync(bool enabled) = 0;
    virtual void setTitle(const std::string& title) = 0;
    
    // Window state
    virtual glm::vec2 getSize() const = 0;
    virtual bool shouldClose() const = 0;
    virtual bool isMinimized() const = 0;
    
    // Events
    virtual void pollEvents() = 0;
    virtual void setEventCallback(std::function<void(Event&)> callback) = 0;
};

// Texture abstraction - separate from context
class Texture {
public:
    virtual ~Texture() = default;
    
    // Texture operations
    virtual void updateData(const void* data, size_t size) = 0;
    virtual void updateRegion(uint32_t x, uint32_t y, uint32_t w, uint32_t h, 
                             const void* data) = 0;
    virtual void* mapData(AccessMode mode) = 0;
    virtual void unmapData() = 0;
    
    // Properties
    virtual uint32_t getWidth() const = 0;
    virtual uint32_t getHeight() const = 0;
    virtual PixelFormat getFormat() const = 0;
    virtual void* getNativeHandle() = 0; // For ImGui integration
    
    // Validation
    virtual bool isValid() const = 0;
    virtual size_t getMemoryUsage() const = 0;
};

// Lightweight coordinator - composition over inheritance
class GraphicsContext {
public:
    GraphicsContext(std::unique_ptr<Window> window, std::unique_ptr<GraphicsAPI> api)
        : m_window(std::move(window)), m_api(std::move(api)) {}
    
    // Component access
    Window& getWindow() { return *m_window; }
    const Window& getWindow() const { return *m_window; }
    GraphicsAPI& getAPI() { return *m_api; }
    const GraphicsAPI& getAPI() const { return *m_api; }
    
    // Convenience methods that delegate
    std::unique_ptr<Texture> createTexture(uint32_t w, uint32_t h, PixelFormat fmt) {
        return m_api->createTexture(w, h, fmt);
    }
    
    ImTextureID getImGuiTextureID(Texture& texture) {
        return (ImTextureID)texture.getNativeHandle();
    }
    
    // Validation
    bool isValid() const { 
        return m_window && m_api && m_window->getNativeWindow() && m_api->isValid(); 
    }
    
    // Factory methods for different platforms
    static std::unique_ptr<GraphicsContext> createOpenGL(uint32_t width, uint32_t height, 
                                                         const std::string& title);
    static std::unique_ptr<GraphicsContext> createMetal(uint32_t width, uint32_t height, 
                                                        const std::string& title);
    
private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<GraphicsAPI> m_api;
};

// Pure rendering parameters - no graphics dependencies
class RenderContext {
public:
    // Configuration
    void setOutputDimensions(uint32_t width, uint32_t height);
    void setSampleCount(uint32_t samples);
    void setMaxBounces(uint32_t bounces);
    void setRenderMode(RenderMode mode);
    void setQualityPreset(QualityPreset preset);
    
    // Progress tracking
    void setProgressCallback(ProgressCallback callback);
    void setCancelCallback(CancelCallback callback);
    
    // Results in CPU memory
    std::shared_ptr<RenderResult> getResult() const;
    RenderStats getStats() const;
    
    // Validation
    bool isValid() const;
    std::vector<std::string> validate() const;
    
private:
    RenderParams m_params;
    std::shared_ptr<RenderResult> m_result;
    RenderStats m_stats;
};
```

### 2. Backend Implementations

#### 2.1 CPU Ray Tracing Backend (Embree-Accelerated)
```cpp
class CPURayTracingBackend : public RenderEngine {
public:
    CPURayTracingBackend();
    ~CPURayTracingBackend();
    
    // RenderEngine interface
    void setScene(std::shared_ptr<Scene> scene) override;
    RenderResult render(const RenderRequest& request) override;
    
private:
    RTCDevice m_device;
    RTCScene m_embreeScene;
    std::unique_ptr<EmbreeSceneBuilder> m_sceneBuilder;
    std::unique_ptr<EmbreeRayTracer> m_rayTracer;
    
    void rebuildAccelerationStructure();
    void updateEmbreeGeometry();
};
```

#### 2.2 Optix Ray Tracing Backend (NVIDIA RTX Hardware)
```cpp
class OptixRayTracingBackend : public RenderEngine {
public:
    OptixRayTracingBackend();
    ~OptixRayTracingBackend();
    
    // RenderEngine interface
    void setScene(std::shared_ptr<Scene> scene) override;
    RenderResult render(const RenderRequest& request) override;
    
private:
    CUcontext m_cudaContext;
    OptixDeviceContext m_optixContext;
    std::unique_ptr<OptixSceneBuilder> m_sceneBuilder;
    std::unique_ptr<OptixPipeline> m_pipeline;
    
    // Resource management
    void initializeOptixContext();
    void cleanupOptixResources();
    bool validateOptixCapabilities() const;
};
```

#### 2.3 Metal Ray Tracing Backend (Apple Silicon Hardware)
```cpp
class MetalRayTracingBackend : public RenderEngine {
public:
    MetalRayTracingBackend();
    ~MetalRayTracingBackend();
    
    // RenderEngine interface
    void setScene(std::shared_ptr<Scene> scene) override;
    RenderResult render(const RenderRequest& request) override;
    
    // Metal-specific capabilities
    bool supportsRayTracing() const;
    std::string getMetalVersion() const;
    
private:
    id<MTLDevice> m_device;
    id<MTLCommandQueue> m_commandQueue;
    id<MTLAccelerationStructure> m_accelerationStructure;
    std::unique_ptr<MetalSceneBuilder> m_sceneBuilder;
    std::unique_ptr<MetalRayTracingPipeline> m_pipeline;
    
    // Resource management
    void initializeMetalContext();
    void cleanupMetalResources();
    bool validateMetalCapabilities() const;
    void buildAccelerationStructure();
};
```

### 3. Post-Processing Pipeline with GPU Support

#### 3.1 Post-Process Pipeline Architecture
```cpp
class PostProcessPipeline {
public:
    PostProcessPipeline(std::shared_ptr<GraphicsContext> context);
    
    // Pipeline management
    void addEffect(std::unique_ptr<PostProcessEffect> effect);
    void removeEffect(const std::string& name);
    void setEffectOrder(const std::vector<std::string>& order);
    
    // Processing - can run on CPU or GPU
    void process(RenderResult& cpuResult);  // CPU processing
    void processGPU(std::shared_ptr<Texture> gpuTexture);  // GPU processing
    
    // Hybrid processing - copy to GPU, process, copy back
    void processHybrid(RenderResult& result);
    
    // Configuration
    void setGlobalParams(const PostProcessParams& params);
    PostProcessEffect* getEffect(const std::string& name);
    
    // Performance settings
    void setPreferGPU(bool prefer) { m_preferGPU = prefer; }
    bool canUseGPU() const { return m_context && m_context->supportsCompute(); }
    
private:
    std::shared_ptr<GraphicsContext> m_context;
    std::vector<std::unique_ptr<PostProcessEffect>> m_effects;
    PostProcessParams m_globalParams;
    bool m_preferGPU = true;
    
    // GPU texture for processing chain
    std::shared_ptr<Texture> m_processingTexture;
};

// Abstract effect base - can run on CPU or GPU
class PostProcessEffect {
public:
    virtual ~PostProcessEffect() = default;
    
    // CPU processing
    virtual void process(RenderResult& result) = 0;
    
    // GPU processing (optional)
    virtual bool supportsGPU() const { return false; }
    virtual void processGPU(std::shared_ptr<Texture> texture, 
                           GraphicsContext& context) {}
    
    // Effect metadata
    virtual std::string getName() const = 0;
    virtual bool isEnabled() const { return m_enabled; }
    virtual void setEnabled(bool enabled) { m_enabled = enabled; }
    
protected:
    bool m_enabled = true;
};

// ACES Tonemap - supports both CPU and GPU
class ACESTonemapEffect : public PostProcessEffect {
public:
    // CPU implementation
    void process(RenderResult& result) override;
    
    // GPU implementation using compute shader
    bool supportsGPU() const override { return true; }
    void processGPU(std::shared_ptr<Texture> texture, 
                   GraphicsContext& context) override;
    
    std::string getName() const override { return "ACES Tonemap"; }
    
    // Parameters
    void setExposure(float exposure) { m_exposure = exposure; }
    void setAutoExposure(bool enabled, float target = 0.18f) { 
        m_autoExposure = enabled; 
        m_targetLuminance = target; 
    }
    
private:
    float m_exposure = 1.0f;
    bool m_autoExposure = false;
    float m_targetLuminance = 0.18f;
    std::shared_ptr<ComputeShader> m_computeShader;  // Cached GPU shader
    
    void ensureComputeShader(GraphicsContext& context);
    const char* getACESShaderSource() const;
};

// Example compute shader for ACES (GLSL)
const char* ACESTonemapEffect::getACESShaderSource() const {
    return R"(
        #version 430
        layout(local_size_x = 16, local_size_y = 16) in;
        layout(rgba32f, binding = 0) uniform image2D u_texture;
        
        uniform float u_exposure;
        
        // ACES tonemap function
        vec3 aces_tonemap(vec3 color) {
            mat3 aces_input_matrix = mat3(
                0.59719, 0.35458, 0.04823,
                0.07600, 0.90834, 0.01566,
                0.02840, 0.13383, 0.83777
            );
            
            mat3 aces_output_matrix = mat3(
                1.60475, -0.53108, -0.07367,
                -0.10208,  1.10813, -0.00605,
                -0.00327, -0.07276,  1.07602
            );
            
            color = aces_input_matrix * color;
            vec3 a = color * (color + 0.0245786) - 0.000090537;
            vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
            color = a / b;
            return aces_output_matrix * color;
        }
        
        void main() {
            ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
            vec4 color = imageLoad(u_texture, coords);
            
            // Apply exposure
            color.rgb *= u_exposure;
            
            // Apply ACES tonemap
            color.rgb = aces_tonemap(color.rgb);
            
            // Clamp to [0,1]
            color.rgb = clamp(color.rgb, 0.0, 1.0);
            
            imageStore(u_texture, coords, color);
        }
    )";
}
```

#### 3.2 Graphics Context Integration Example
```cpp
// In your application setup
std::shared_ptr<GraphicsContext> context = std::make_shared<SDLOpenGLContext>(
    1920, 1080, "PathTracer Editor");

// Create post-processing pipeline with shared context
auto postProcess = std::make_unique<PostProcessPipeline>(context);
postProcess->addEffect(std::make_unique<ACESTonemapEffect>());
postProcess->addEffect(std::make_unique<GammaCorrectEffect>());

// Create renderer that outputs to CPU memory
auto renderer = std::make_unique<CPURayTracingBackend>();

// Create display texture that both UI and post-processing can use
auto displayTexture = context->createTexture(512, 512, PixelFormat::RGBA32F);

// Rendering pipeline:
void renderFrame() {
    // 1. Render to CPU memory
    RenderResult cpuResult = renderer->render(renderRequest);
    
    // 2. Either process on CPU or GPU
    if (postProcess->canUseGPU() && settings.useGPUPostProcessing) {
        // Upload to GPU texture
        context->updateTexture(displayTexture, cpuResult.data());
        // Process on GPU
        postProcess->processGPU(displayTexture);
        // Texture is now ready for ImGui display
    } else {
        // Process on CPU
        postProcess->process(cpuResult);
        // Upload final result to display texture
        context->updateTexture(displayTexture, cpuResult.data());
    }
    
    // 3. Display in ImGui
    ImTextureID texID = context->getImGuiTextureID(displayTexture);
    ImGui::Image(texID, ImVec2(512, 512));
}
```

#### 3.3 Why Shared Graphics Context Solves Everything

**The Problem**: Separate contexts can't share textures
- UI context (SDL/ImGui) creates OpenGL textures
- Renderer context creates its own textures  
- **Result**: Can't pass textures between them, must copy via CPU memory

**The Solution**: Single shared graphics context
- **One context** creates window, OpenGL context, handles all textures
- **UI system** uses context for ImGui rendering and display
- **Post-processing** uses same context for compute shaders
- **Renderer** outputs to CPU, then uploads to shared textures
- **Result**: Zero-copy texture sharing, GPU post-processing works seamlessly

**Example Flow**:
```cpp
// 1. Create single graphics context
auto context = std::make_shared<SDLOpenGLContext>(1920, 1080, "PathTracer");

// 2. Initialize all systems with shared context
PathTracerEditor editor(context);
PostProcessPipeline postProcess(context);
UIManager ui(context);

// 3. Create shared textures
auto renderTexture = context->createTexture(512, 512, PixelFormat::RGBA32F);

// 4. Everyone can use the same texture!
// - Renderer uploads data to it
// - Post-processing processes it in-place with compute shaders
// - UI displays it directly with ImGui
// - No copying between contexts needed!
```

**Platform-Specific Benefits**:
- **OpenGL**: Compute shaders for post-processing, shared with ImGui textures
- **Metal**: Metal Performance Shaders + native ImGui Metal backend
- **Vulkan**: Vulkan compute + ImGui Vulkan backend (future)

### 4. Applications Layer

#### 4.1 PathTracer Editor (GUI)
```cpp
class PathTracerEditor {
public:
    PathTracerEditor();
    ~PathTracerEditor();
    
    void run();
    
private:
    // Core systems
    std::unique_ptr<RenderEngine> m_renderEngine;
    std::shared_ptr<Scene> m_scene;
    std::unique_ptr<PostProcessPipeline> m_postProcess;
    
    // UI systems  
    std::unique_ptr<UIManager> m_ui;
    std::unique_ptr<ViewportManager> m_viewport;
    std::unique_ptr<DisplayContext> m_displayContext;  // SDL/OpenGL/etc
    
    // Editor functionality
    void setupUI();
    void updateViewport();
    void handleInput();
    void renderFrame();
};
```

#### 4.2 PathTracer CLI
```cpp
class PathTracerCLI {
public:
    static int main(int argc, char** argv);
    
private:
    struct CLIConfig {
        std::string sceneFile;
        std::string outputFile;
        uint32_t width, height;
        uint32_t samples;
        std::string backend;
        PostProcessConfig postProcess;
    };
    
    static CLIConfig parseArguments(int argc, char** argv);
    static void renderScene(const CLIConfig& config);
};
```

## 🔄 Migration Game Plan

### Phase 1: Core Library Extraction (Week 1-2)

#### 1.1 Create Library Structure
- [ ] Create `libs/pathtracer/` directory structure
- [ ] Set up CMake for library build (`libpathtracer.lib/.a`)
- [ ] Create public API headers in `libs/pathtracer/include/`
- [ ] Create implementation in `libs/pathtracer/src/`

#### 1.2 Extract Core Components
- [ ] Move `Scene` class to library, remove Embree coupling
- [ ] Create `RenderEngine` interface
- [ ] Extract camera system from current code
- [ ] Create `RenderResult` and `RenderRequest` data structures

#### 1.3 Update Build System
```cmake
# libs/pathtracer/CMakeLists.txt
add_library(pathtracer STATIC
    src/Scene.cpp
    src/Camera.cpp
    src/RenderEngine.cpp
    src/Materials.cpp
    # ... other sources
)

target_include_directories(pathtracer PUBLIC
    include/
)

target_link_libraries(pathtracer PUBLIC
    glm
    # Core dependencies only
)
```

### Phase 2: Backend Abstraction (Week 2-3)

#### 2.1 Create Backend Interface
- [ ] Design `RenderBackend` abstract base class
- [ ] Define capability query system
- [ ] Create backend factory/registry pattern

#### 2.2 Refactor Embree Implementation
- [ ] Extract Embree-specific code from current `EmbreeRenderTarget`
- [ ] Implement `EmbreeRenderBackend` using new interface
- [ ] Separate path tracing logic from display concerns
- [ ] Move Embree dependencies to backend-specific module

#### 2.3 Scene Builder Abstraction
- [ ] Create `SceneBuilder` interface for backend-specific scene construction
- [ ] Implement `EmbreeSceneBuilder`
- [ ] Handle scene change tracking and incremental updates

### Phase 3: Display Context Separation (Week 3-4)

#### 3.1 Create Display Abstraction
- [ ] Design `DisplayContext` interface for UI rendering
- [ ] Implement `SDLDisplayContext` for current SDL usage
- [ ] Create `Texture` and `TextureManager` for display textures
- [ ] Separate rendering output from display textures

#### 3.2 Renderer-Display Bridge
```cpp
class RenderDisplayBridge {
public:
    void updateDisplayTexture(const RenderResult& result);
    void setDisplayContext(std::shared_ptr<DisplayContext> context);
    ImTextureID getDisplayTexture() const;
    
private:
    std::shared_ptr<DisplayContext> m_displayContext;
    std::unique_ptr<Texture> m_displayTexture;
    std::unique_ptr<FormatConverter> m_converter;
};
```

### Phase 4: Post-Processing Pipeline (Week 4-5)

#### 4.1 Create Post-Process Framework
- [ ] Design `PostProcessEffect` base class
- [ ] Implement `PostProcessPipeline` manager
- [ ] Create effect registration system

#### 4.2 Implement Core Effects
- [ ] `ACESTonemapEffect` with auto-exposure
- [ ] `GammaCorrectEffect`
- [ ] `BloomEffect` (future)
- [ ] `DenoiseEffect` (future)

#### 4.3 Integrate with Renderer
- [ ] Add post-processing to render pipeline
- [ ] Create UI controls for effect parameters
- [ ] Implement real-time preview

### Phase 5: Application Restructure (Week 5-6)

#### 5.1 Create Editor Application
- [ ] Move current `App` class to `apps/editor/`
- [ ] Refactor to use pathtracer library
- [ ] Implement new display context system
- [ ] Create modular UI system

#### 5.2 UI System Redesign
```cpp
// Modular UI components
class ViewportPanel : public UIPanel {
public:
    void render() override;
    void setRenderResult(const RenderResult& result);
};

class SceneHierarchyPanel : public UIPanel {
public:
    void render() override;
    void setScene(std::shared_ptr<Scene> scene);
};

class RenderSettingsPanel : public UIPanel {
public:
    void render() override;
    void setRenderEngine(RenderEngine* engine);
};
```

#### 5.3 Editor Project Structure
```
apps/
└── editor/
    ├── CMakeLists.txt
    ├── src/
    │   ├── main.cpp
    │   ├── PathTracerEditor.cpp
    │   ├── ui/
    │   │   ├── UIManager.cpp
    │   │   ├── ViewportPanel.cpp
    │   │   ├── ScenePanel.cpp
    │   │   └── SettingsPanel.cpp
    │   └── display/
    │       ├── SDLDisplayContext.cpp
    │       └── ViewportManager.cpp
    └── include/
        └── editor/
            ├── PathTracerEditor.h
            └── ui/
                └── UIManager.h
```

### Phase 6: CLI Application (Week 6-7)

#### 6.1 Create CLI Interface
- [ ] Design command-line argument parsing
- [ ] Implement batch rendering functionality
- [ ] Add progress reporting for headless operation

#### 6.2 CLI Features
```bash
# Basic rendering
pathtracer-cli scene.json --output render.exr --size 1920x1080 --samples 1024

# Backend selection
pathtracer-cli scene.json --backend embree --output render.exr

# Post-processing
pathtracer-cli scene.json --tonemap aces --exposure 1.2 --gamma 2.2

# Progressive rendering with preview
pathtracer-cli scene.json --progressive --preview-interval 100
```

#### 6.3 CLI Project Structure
```
apps/
└── cli/
    ├── CMakeLists.txt
    ├── src/
    │   ├── main.cpp
    │   ├── PathTracerCLI.cpp
    │   ├── ArgumentParser.cpp
    │   └── ProgressReporter.cpp
    └── include/
        └── cli/
            └── PathTracerCLI.h
```

### Phase 7: Future Backend Support (Week 7+)

#### 7.1 Optix Backend Preparation
- [ ] Create `OptixRenderBackend` skeleton
- [ ] Design CUDA memory management
- [ ] Implement Optix pipeline setup
- [ ] Create Optix-specific scene builder

#### 7.2 Metal Backend Preparation (macOS)
- [ ] Research Metal Performance Shaders for ray tracing
- [ ] Design Metal compute pipeline
- [ ] Create Metal-specific resource management

#### 7.3 Backend Plugin System
```cpp
class BackendRegistry {
public:
    static void registerBackend(const std::string& name, 
                               BackendFactory factory);
    static std::unique_ptr<RenderEngine> createBackend(
                               const std::string& name);
    static std::vector<std::string> getAvailableBackends();
};

// Plugin loading
#ifdef OPTIX_AVAILABLE
    BackendRegistry::registerBackend("optix", OptixBackend::create);
#endif
#ifdef METAL_AVAILABLE
    BackendRegistry::registerBackend("metal", MetalBackend::create);
#endif
```

## 📦 Final Project Structure

```
software-path-tracer/
├── CMakeLists.txt                 # Root build configuration
├── libs/                          # Core libraries
│   └── pathtracer/
│       ├── CMakeLists.txt
│       ├── include/pathtracer/    # Public API
│       │   ├── RenderEngine.h
│       │   ├── Scene.h
│       │   ├── Camera.h
│       │   ├── PostProcess.h
│       │   └── Types.h
│       └── src/                   # Implementation
│           ├── core/
│           │   ├── Scene.cpp
│           │   ├── Camera.cpp
│           │   └── Materials.cpp
│           ├── backends/
│           │   ├── embree/
│           │   │   ├── EmbreeRenderBackend.cpp
│           │   │   └── EmbreeSceneBuilder.cpp
│           │   ├── optix/         # Future
│           │   └── metal/         # Future
│           └── postprocess/
│               ├── PostProcessPipeline.cpp
│               ├── ACESTonemapEffect.cpp
│               └── GammaCorrectEffect.cpp
├── apps/                          # Applications
│   ├── editor/                    # GUI Editor
│   │   ├── CMakeLists.txt
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── PathTracerEditor.cpp
│   │   │   ├── ui/
│   │   │   └── display/
│   │   └── include/
│   └── cli/                       # CLI Tool
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── main.cpp
│       │   └── PathTracerCLI.cpp
│       └── include/
├── vendor/                        # Third-party dependencies
│   ├── embree/
│   ├── sdl/
│   ├── imgui/
│   └── glm/
├── assets/                        # Test scenes and resources
├── docs/                          # Documentation
│   ├── api/                       # API documentation
│   ├── backends/                  # Backend-specific docs
│   └── examples/                  # Usage examples
└── tests/                         # Unit and integration tests
    ├── unit/
    ├── integration/
    └── benchmarks/
```

## 🎛️ Advanced Features & Considerations

### Scene File Format
Design a JSON-based scene format for CLI usage:
```json
{
  "scene": {
    "camera": {
      "position": [0, 0, 5],
      "target": [0, 0, 0],
      "fov": 45
    },
    "environment": {
      "type": "hdri",
      "path": "assets/environment.exr",
      "intensity": 1.0
    },
    "objects": [
      {
        "type": "sphere",
        "center": [0, 0, 0],
        "radius": 1.0,
        "material": "red_metal"
      }
    ],
    "materials": {
      "red_metal": {
        "type": "metallic",
        "albedo": [0.8, 0.2, 0.2],
        "roughness": 0.1,
        "metallic": 1.0
      }
    }
  },
  "render": {
    "samples": 1024,
    "maxBounces": 8,
    "resolution": [1920, 1080]
  },
  "postProcess": {
    "tonemap": "aces",
    "exposure": 1.0,
    "gamma": 2.2
  }
}
```

## 🛡️ Production-Quality Engineering

### 🧪 Testing Strategy

#### Unit Testing Framework
```cpp
// Google Test for comprehensive unit testing
class CPURayTracingBackendTest : public ::testing::Test {
protected:
    void SetUp() override {
        backend = std::make_unique<CPURayTracingBackend>();
        scene = std::make_shared<Scene>();
        setupTestScene();
    }
    
    void TearDown() override {
        backend.reset();
        scene.reset();
    }
    
    void setupTestScene() {
        // Create reproducible test scene
        scene->addSphere({0, 0, 0}, 1.0f);
        scene->setEnvironmentMap(createTestEnvironment());
    }
    
    std::unique_ptr<CPURayTracingBackend> backend;
    std::shared_ptr<Scene> scene;
};

TEST_F(CPURayTracingBackendTest, RendersSingleSphere) {
    RenderRequest request;
    request.width = 64;
    request.height = 64;
    request.samples = 1;
    
    RenderResult result = backend->render(request);
    
    EXPECT_TRUE(result.isValid());
    EXPECT_EQ(result.getWidth(), 64);
    EXPECT_EQ(result.getHeight(), 64);
    // Validate specific pixel values for known scene
    EXPECT_NEAR(result.getPixel(32, 32).r, 0.8f, 0.1f); // Sphere center
}
```

#### Integration Testing
```cpp
class EndToEndRenderTest : public ::testing::Test {
public:
    void TestRenderPipeline(const std::string& backendName) {
        // Test complete pipeline: Scene -> Render -> PostProcess -> Display
        auto context = GraphicsContext::createOpenGL(512, 512, "Test");
        auto backend = BackendRegistry::createBackend(backendName);
        auto postProcess = std::make_unique<PostProcessPipeline>(context);
        
        // Load test scene
        Scene scene;
        scene.loadFromFile("test_assets/cornell_box.json");
        backend->setScene(std::make_shared<Scene>(scene));
        
        // Render
        RenderRequest request;
        request.width = 256;
        request.height = 256;
        request.samples = 64;
        
        RenderResult result = backend->render(request);
        
        // Post-process
        postProcess->addEffect(std::make_unique<ACESTonemapEffect>());
        postProcess->process(result);
        
        // Validate final output
        validateRenderResult(result, "expected_cornell_box.exr");
    }
};

TEST_F(EndToEndRenderTest, CPUBackendProducesCorrectOutput) {
    TestRenderPipeline("cpu_raytracing");
}

TEST_F(EndToEndRenderTest, OptixBackendProducesCorrectOutput) {
    if (OptixRayTracingBackend::isSupported()) {
        TestRenderPipeline("optix_raytracing");
    } else {
        GTEST_SKIP() << "Optix not supported on this system";
    }
}
```

#### Performance/Benchmark Testing
```cpp
class RenderPerformanceTest : public ::testing::Test {
public:
    void BenchmarkRenderTime(const std::string& sceneName, 
                           const std::string& backendName) {
        auto backend = BackendRegistry::createBackend(backendName);
        auto scene = Scene::loadFromFile("benchmarks/" + sceneName + ".json");
        backend->setScene(scene);
        
        RenderRequest request;
        request.width = 1920;
        request.height = 1080;
        request.samples = 1024;
        
        auto start = std::chrono::high_resolution_clock::now();
        RenderResult result = backend->render(request);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Log performance metrics
        PATHTRACER_LOG_INFO("Backend: {} Scene: {} Time: {}ms", 
                           backendName, sceneName, duration.count());
        
        // Performance regression testing
        EXPECT_LT(duration.count(), getPerformanceBaseline(sceneName, backendName) * 1.1);
    }
};
```

#### Test Organization
```
tests/
├── unit/                          # Fast, isolated tests
│   ├── core/
│   │   ├── test_scene.cpp
│   │   ├── test_camera.cpp
│   │   └── test_materials.cpp
│   ├── backends/
│   │   ├── test_cpu_backend.cpp
│   │   ├── test_optix_backend.cpp
│   │   └── test_metal_backend.cpp
│   └── postprocess/
│       ├── test_tonemap.cpp
│       └── test_gamma.cpp
├── integration/                   # End-to-end tests
│   ├── test_render_pipeline.cpp
│   ├── test_scene_loading.cpp
│   └── test_cli_interface.cpp
├── benchmarks/                    # Performance tests
│   ├── test_render_performance.cpp
│   ├── test_memory_usage.cpp
│   └── test_scalability.cpp
├── visual/                        # Visual regression tests
│   ├── reference_images/
│   ├── test_visual_output.cpp
│   └── image_comparison.cpp
└── test_assets/                   # Test scenes and data
    ├── simple_sphere.json
    ├── cornell_box.json
    └── complex_scene.json
```

### 📊 Logging and Diagnostics

#### Structured Logging System
```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger {
public:
    static void initialize() {
        // Console sink with color
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::info);
        
        // Rotating file sink
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/pathtracer.log", 1048576 * 5, 3);
        file_sink->set_level(spdlog::level::debug);
        
        // Create logger with both sinks
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("pathtracer", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        
        spdlog::register_logger(logger);
        spdlog::set_default_logger(logger);
    }
    
    static void shutdown() {
        spdlog::shutdown();
    }
};

// Convenient macros
#define PATHTRACER_LOG_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define PATHTRACER_LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define PATHTRACER_LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define PATHTRACER_LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define PATHTRACER_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define PATHTRACER_LOG_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

// Usage examples
PATHTRACER_LOG_INFO("Starting render: {}x{} samples:{}", width, height, samples);
PATHTRACER_LOG_WARN("GPU backend unavailable, falling back to CPU");
PATHTRACER_LOG_ERROR("Failed to load scene: {}", filename);
```

#### Performance Profiling
```cpp
class ProfilerBlock {
public:
    ProfilerBlock(const std::string& name) : m_name(name) {
        m_start = std::chrono::high_resolution_clock::now();
    }
    
    ~ProfilerBlock() {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
        PATHTRACER_LOG_DEBUG("Profile: {} took {}μs", m_name, duration.count());
    }
    
private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_start;
};

#define PROFILE_SCOPE(name) ProfilerBlock _prof(name)
#define PROFILE_FUNCTION() ProfilerBlock _prof(__FUNCTION__)

// Usage
void CPURayTracingBackend::render(const RenderRequest& request) {
    PROFILE_FUNCTION();
    
    {
        PROFILE_SCOPE("Scene Preparation");
        rebuildAccelerationStructure();
    }
    
    {
        PROFILE_SCOPE("Ray Tracing");
        performRayTracing(request);
    }
}
```

### 🔧 Error Handling and Resilience

#### Result Type for Error Handling
```cpp
template<typename T, typename E = std::string>
class Result {
public:
    static Result success(T value) {
        Result r;
        r.m_success = true;
        r.m_value = std::move(value);
        return r;
    }
    
    static Result error(E error) {
        Result r;
        r.m_success = false;
        r.m_error = std::move(error);
        return r;
    }
    
    bool isSuccess() const { return m_success; }
    bool isError() const { return !m_success; }
    
    const T& getValue() const { 
        if (!m_success) throw std::runtime_error("Accessing value on error result");
        return m_value; 
    }
    
    const E& getError() const {
        if (m_success) throw std::runtime_error("Accessing error on success result");
        return m_error;
    }
    
private:
    bool m_success;
    T m_value;
    E m_error;
};

// Usage
Result<RenderResult> CPURayTracingBackend::render(const RenderRequest& request) {
    if (!validateRequest(request)) {
        return Result<RenderResult>::error("Invalid render request parameters");
    }
    
    if (!m_device) {
        return Result<RenderResult>::error("Embree device not initialized");
    }
    
    try {
        RenderResult result = performRender(request);
        return Result<RenderResult>::success(std::move(result));
    } catch (const std::exception& e) {
        PATHTRACER_LOG_ERROR("Render failed: {}", e.what());
        return Result<RenderResult>::error(e.what());
    }
}
```

#### Graceful Degradation
```cpp
class BackendRegistry {
public:
    static std::unique_ptr<RenderEngine> createBestAvailableBackend() {
        // Try hardware ray tracing first
        if (MetalRayTracingBackend::isSupported()) {
            PATHTRACER_LOG_INFO("Using Metal ray tracing backend");
            return std::make_unique<MetalRayTracingBackend>();
        }
        
        if (OptixRayTracingBackend::isSupported()) {
            PATHTRACER_LOG_INFO("Using Optix ray tracing backend");
            return std::make_unique<OptixRayTracingBackend>();
        }
        
        // Fallback to CPU
        PATHTRACER_LOG_WARN("Hardware ray tracing unavailable, using CPU backend");
        return std::make_unique<CPURayTracingBackend>();
    }
    
    static bool validateBackendCapabilities(const std::string& backendName) {
        auto backend = createBackend(backendName);
        if (!backend) return false;
        
        try {
            auto caps = backend->getCapabilities();
            return caps.isValid();
        } catch (...) {
            return false;
        }
    }
};
```

### 💾 Memory Management and Resource Safety

#### Smart Pointer Strategy
```cpp
// Scene objects: shared ownership
std::shared_ptr<Scene> scene;
std::shared_ptr<Material> material;
std::shared_ptr<Geometry> geometry;

// Backend implementations: unique ownership
std::unique_ptr<RenderEngine> backend;
std::unique_ptr<PostProcessEffect> effect;

// GPU resources: RAII wrappers
class CUDAMemoryBlock {
public:
    CUDAMemoryBlock(size_t size) {
        if (cudaMalloc(&m_ptr, size) != cudaSuccess) {
            throw std::runtime_error("CUDA allocation failed");
        }
        m_size = size;
    }
    
    ~CUDAMemoryBlock() {
        if (m_ptr) {
            cudaFree(m_ptr);
        }
    }
    
    // Non-copyable, movable
    CUDAMemoryBlock(const CUDAMemoryBlock&) = delete;
    CUDAMemoryBlock& operator=(const CUDAMemoryBlock&) = delete;
    
    CUDAMemoryBlock(CUDAMemoryBlock&& other) noexcept
        : m_ptr(other.m_ptr), m_size(other.m_size) {
        other.m_ptr = nullptr;
        other.m_size = 0;
    }
    
    void* get() const { return m_ptr; }
    size_t size() const { return m_size; }
    
private:
    void* m_ptr = nullptr;
    size_t m_size = 0;
};
```

#### Resource Pool Management
```cpp
template<typename T>
class ResourcePool {
public:
    template<typename... Args>
    std::shared_ptr<T> acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_available.empty()) {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
        
        auto resource = m_available.back();
        m_available.pop_back();
        return resource;
    }
    
    void release(std::shared_ptr<T> resource) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_available.size() < m_maxPoolSize) {
            m_available.push_back(resource);
        }
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available.clear();
    }
    
private:
    std::vector<std::shared_ptr<T>> m_available;
    std::mutex m_mutex;
    size_t m_maxPoolSize = 32;
};

// Global texture pool
ResourcePool<Texture> g_texturePool;
```

### 🔧 Configuration Management

#### Hierarchical Configuration System
```cpp
class ConfigManager {
public:
    static ConfigManager& instance() {
        static ConfigManager s_instance;
        return s_instance;
    }
    
    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            PATHTRACER_LOG_WARN("Config file not found: {}, using defaults", filename);
            return;
        }
        
        nlohmann::json j;
        file >> j;
        m_config = j;
        
        PATHTRACER_LOG_INFO("Loaded configuration from {}", filename);
    }
    
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const {
        try {
            return m_config.at(key).get<T>();
        } catch (...) {
            return defaultValue;
        }
    }
    
    void set(const std::string& key, const nlohmann::json& value) {
        m_config[key] = value;
    }
    
    void saveToFile(const std::string& filename) const {
        std::ofstream file(filename);
        file << m_config.dump(4);
        PATHTRACER_LOG_INFO("Saved configuration to {}", filename);
    }
    
private:
    nlohmann::json m_config;
};

// Usage
auto& config = ConfigManager::instance();
config.loadFromFile("config.json");

int samples = config.get<int>("render.samples", 1024);
float exposure = config.get<float>("postprocess.exposure", 1.0f);
bool useGPU = config.get<bool>("render.preferGPU", true);
```

### 📈 Performance Monitoring and Telemetry

#### Metrics Collection
```cpp
class MetricsCollector {
public:
    void recordRenderTime(const std::string& backend, double timeMs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_renderTimes[backend].push_back(timeMs);
        
        // Sliding window of last 100 renders
        if (m_renderTimes[backend].size() > 100) {
            m_renderTimes[backend].erase(m_renderTimes[backend].begin());
        }
    }
    
    void recordMemoryUsage(size_t bytesUsed) {
        m_currentMemoryUsage = bytesUsed;
        m_peakMemoryUsage = std::max(m_peakMemoryUsage, bytesUsed);
    }
    
    double getAverageRenderTime(const std::string& backend) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_renderTimes.find(backend);
        if (it == m_renderTimes.end() || it->second.empty()) {
            return 0.0;
        }
        
        double sum = std::accumulate(it->second.begin(), it->second.end(), 0.0);
        return sum / it->second.size();
    }
    
    void generateReport() const {
        PATHTRACER_LOG_INFO("=== Performance Report ===");
        for (const auto& [backend, times] : m_renderTimes) {
            PATHTRACER_LOG_INFO("Backend {}: avg {:.2f}ms, samples: {}", 
                               backend, getAverageRenderTime(backend), times.size());
        }
        PATHTRACER_LOG_INFO("Memory: current {:.2f}MB, peak {:.2f}MB",
                           m_currentMemoryUsage / 1024.0 / 1024.0,
                           m_peakMemoryUsage / 1024.0 / 1024.0);
    }
    
private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::vector<double>> m_renderTimes;
    size_t m_currentMemoryUsage = 0;
    size_t m_peakMemoryUsage = 0;
};
```

### ⚡ Template Metaprogramming Optimizations

#### Compile-Time Material System
```cpp
// Zero-runtime-cost material dispatch
template<MaterialType Type>
struct MaterialTraits;

template<>
struct MaterialTraits<MaterialType::Lambert> {
    using Parameters = LambertParameters;
    static constexpr bool hasSpecular = false;
    static constexpr const char* shaderName = "lambert";
};

template<MaterialType Type>
class Material {
public:
    using Params = typename MaterialTraits<Type>::Parameters;
    static constexpr const char* getShaderName() {
        return MaterialTraits<Type>::shaderName;
    }
    
    const Params& getParameters() const { return m_params; }
    
private:
    Params m_params;
};

// Usage: Compiler eliminates all abstraction overhead
auto metal = Material<MaterialType::Metal>(0.8f, 0.1f);
```

#### SIMD Ray Packet Processing
```cpp
// Compile-time SIMD width optimization
template<int PacketSize>
struct SIMDTraits;

template<> struct SIMDTraits<4> {
    using Type = __m128;
    static constexpr auto load = _mm_load_ps;
    static constexpr auto add = _mm_add_ps;
    static constexpr auto mul = _mm_mul_ps;
};

template<> struct SIMDTraits<8> {
    using Type = __m256;
    static constexpr auto load = _mm256_load_ps;
    static constexpr auto add = _mm256_add_ps;
    static constexpr auto mul = _mm256_mul_ps;
};

template<int PacketSize>
class RayPacket {
    using SIMD = SIMDTraits<PacketSize>;
    
public:
    void intersectSphere(const Sphere& sphere) {
        // Compiler generates optimal SIMD for target CPU
        typename SIMD::Type originX = SIMD::load(&m_originX[0]);
        // ... vectorized intersection math
    }
    
private:
    alignas(32) float m_originX[PacketSize];
    // ... other ray components
};

// Compile-time specialization for target CPU
using OptimalRayPacket = std::conditional_t<
    has_avx512(), RayPacket<16>,
    std::conditional_t<has_avx(), RayPacket<8>, RayPacket<4>>
>;
```

#### Backend Capability Detection
```cpp
// Compile-time feature detection using concepts
template<typename Backend>
concept HasHardwareRT = requires(Backend b) {
    b.traceRaysHardware();
};

template<typename Backend>
concept HasDenoiser = requires(Backend b) {
    b.denoise();
};

template<typename Backend>
class RenderPipeline {
public:
    void render(const Scene& scene) {
        if constexpr (HasHardwareRT<Backend>) {
            m_backend.traceRaysHardware();
        } else {
            m_backend.traceRaysSoftware();
        }
        
        if constexpr (HasDenoiser<Backend>) {
            m_backend.denoise();
        }
    }
    
private:
    Backend m_backend;
};

// Zero-cost backend selection
template<GraphicsAPI API>
auto createOptimalBackend() {
    if constexpr (API == GraphicsAPI::OptiX) {
        return OptixRayTracingBackend{};
    } else if constexpr (API == GraphicsAPI::Metal) {
        return MetalRayTracingBackend{};
    } else {
        return CPURayTracingBackend{};
    }
}
```

#### Type-Safe Compute Shader Parameters
```cpp
// Compile-time shader parameter validation
template<typename... Params>
struct ComputeShaderSignature {
    static constexpr size_t paramCount = sizeof...(Params);
    using ParamTuple = std::tuple<Params...>;
};

using ACESShaderSig = ComputeShaderSignature<
    float,        // exposure
    uint32_t,     // width
    uint32_t,     // height  
    Texture*      // texture
>;

template<typename Signature>
class TypedComputeShader {
public:
    template<typename... Args>
    void dispatch(Args&&... args) {
        static_assert(sizeof...(Args) == Signature::paramCount);
        // Compile-time type validation
        validateParameters<0>(std::forward<Args>(args)...);
        dispatchImpl(std::forward<Args>(args)...);
    }
    
private:
    template<size_t Index, typename First, typename... Rest>
    void validateParameters(First&& first, Rest&&... rest) {
        using Expected = std::tuple_element_t<Index, typename Signature::ParamTuple>;
        static_assert(std::is_same_v<std::decay_t<First>, Expected>);
        
        if constexpr (sizeof...(Rest) > 0) {
            validateParameters<Index + 1>(std::forward<Rest>(rest)...);
        }
    }
};

// Usage: Compile error if wrong parameter types
auto acesShader = TypedComputeShader<ACESShaderSig>{};
acesShader.dispatch(1.0f, 512u, 512u, texture); // ✅ 
// acesShader.dispatch(1.0f, 512); // ❌ Compile error
```

#### Performance Benefits
- **Zero Runtime Cost**: All dispatch resolved at compile time
- **Type Safety**: Catch errors at compile time, not runtime
- **SIMD Optimization**: Compiler generates optimal code for target CPU
- **Memory Layout**: Template parameters control memory alignment
- **Inlining**: No virtual function overhead, everything inlined

### 🔒 Thread Safety and Concurrency

#### Thread-Safe Components
```cpp
class ThreadSafeScene {
public:
    void addGeometry(std::shared_ptr<Geometry> geometry) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_geometries.push_back(geometry);
        m_changeCounter++;
    }
    
    std::vector<std::shared_ptr<Geometry>> getGeometries() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_geometries; // Copy for thread safety
    }
    
    uint64_t getChangeCounter() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_changeCounter;
    }
    
private:
    mutable std::shared_mutex m_mutex;
    std::vector<std::shared_ptr<Geometry>> m_geometries;
    uint64_t m_changeCounter = 0;
};
```

### 🎯 Quality Assurance Checklist

#### Code Quality Standards
- [ ] **Static Analysis**: clang-tidy, PVS-Studio integration
- [ ] **Code Coverage**: Minimum 80% line coverage, 70% branch coverage
- [ ] **Documentation**: Doxygen comments for all public APIs
- [ ] **Code Review**: All changes require review + approval
- [ ] **Formatting**: clang-format with consistent style
- [ ] **Memory Safety**: Address Sanitizer + Valgrind on CI
- [ ] **Thread Safety**: Thread Sanitizer on multithreaded code

#### Release Criteria
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Performance benchmarks within 5% of baseline
- [ ] Memory usage under limits
- [ ] No memory leaks detected
- [ ] All target platforms tested
- [ ] Documentation updated
- [ ] Changelog updated

### 🚀 CI/CD and Automation

#### GitHub Actions Workflow
```yaml
name: PathTracer CI/CD
on:
  push:
    branches: [ main, develop ]
  pull_request:
    branches: [ main ]

jobs:
  test:
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        build_type: [Debug, Release]
        include:
          - os: ubuntu-latest
            cc: gcc-11
            cxx: g++-11
          - os: windows-latest
            cc: cl
            cxx: cl
          - os: macos-latest
            cc: clang
            cxx: clang++
    
    runs-on: ${{ matrix.os }}
    
    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive
    
    - name: Setup vcpkg
      uses: lukka/run-vcpkg@v11
      with:
        vcpkgDirectory: '${{ github.workspace }}/vcpkg'
        vcpkgGitCommitId: '2023.08.09'
    
    - name: Configure CMake
      run: |
        cmake -B build \
              -DCMAKE_BUILD_TYPE=${{ matrix.build_type }} \
              -DCMAKE_TOOLCHAIN_FILE=${{ github.workspace }}/vcpkg/scripts/buildsystems/vcpkg.cmake \
              -DENABLE_TESTING=ON \
              -DENABLE_COVERAGE=${{ matrix.build_type == 'Debug' }} \
              -DENABLE_SANITIZERS=${{ matrix.build_type == 'Debug' }}
    
    - name: Build
      run: cmake --build build --config ${{ matrix.build_type }} --parallel
    
    - name: Test
      run: |
        cd build
        ctest --output-on-failure --parallel --build-config ${{ matrix.build_type }}
    
    - name: Coverage Report (Linux Debug only)
      if: matrix.os == 'ubuntu-latest' && matrix.build_type == 'Debug'
      run: |
        sudo apt-get install lcov
        lcov --capture --directory build --output-file coverage.info
        lcov --remove coverage.info '/usr/*' --output-file coverage.info
        lcov --list coverage.info
    
    - name: Upload Coverage to Codecov
      if: matrix.os == 'ubuntu-latest' && matrix.build_type == 'Debug'
      uses: codecov/codecov-action@v3
      with:
        file: ./coverage.info
        fail_ci_if_error: true

  static-analysis:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    
    - name: Install clang-tidy
      run: sudo apt-get install clang-tidy-14
    
    - name: Run clang-tidy
      run: |
        cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
        find src -name "*.cpp" -exec clang-tidy-14 {} -p build \;

  performance-regression:
    runs-on: ubuntu-latest
    needs: test
    if: github.event_name == 'pull_request'
    steps:
    - uses: actions/checkout@v3
      with:
        fetch-depth: 0  # Need full history for comparison
    
    - name: Build and Benchmark
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_BENCHMARKS=ON
        cmake --build build --parallel
        cd build && ctest -R "benchmark" --output-on-failure
    
    - name: Compare with baseline
      run: |
        # Compare current performance with main branch baseline
        python scripts/compare_benchmarks.py build/benchmark_results.json baseline_benchmarks.json

  package:
    runs-on: ${{ matrix.os }}
    needs: [test, static-analysis]
    if: github.ref == 'refs/heads/main'
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Build Release Package
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_PACKAGING=ON
        cmake --build build --target package
    
    - name: Upload Artifacts
      uses: actions/upload-artifact@v3
      with:
        name: pathtracer-${{ matrix.os }}
        path: build/packages/*
```

#### Development Automation
```bash
#!/bin/bash
# scripts/dev-setup.sh - One-command development environment setup

set -e

echo "🚀 Setting up PathTracer development environment..."

# Install vcpkg if not present
if [ ! -d "vcpkg" ]; then
    git clone https://github.com/Microsoft/vcpkg.git
    ./vcpkg/bootstrap-vcpkg.sh
fi

# Install dependencies
./vcpkg/vcpkg install sdl3 glm embree3 openimageio gtest spdlog

# Configure CMake with development settings
cmake -B build \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DENABLE_TESTING=ON \
      -DENABLE_SANITIZERS=ON \
      -DENABLE_CLANG_TIDY=ON

# Build project
cmake --build build --parallel

# Run tests to verify setup
cd build && ctest --output-on-failure

echo "✅ Development environment ready!"
echo "💡 Try: cmake --build build && ./build/bin/pathtracer-editor"
```

#### Pre-commit Hooks
```bash
#!/bin/bash
# .git/hooks/pre-commit - Ensure code quality before commits

echo "🔍 Running pre-commit checks..."

# Format code
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run static analysis on changed files
CHANGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|h)$')
if [ ! -z "$CHANGED_FILES" ]; then
    clang-tidy $CHANGED_FILES -p build/ || exit 1
fi

# Run quick tests
cd build && ctest -j$(nproc) --timeout 30 || exit 1

echo "✅ Pre-commit checks passed"
```

## 🚀 Getting Started

### Immediate Next Steps
1. **Week 1**: Start with Phase 1 - create the basic library structure
2. **Quick Win**: Move Scene class to library first (least dependencies)
3. **Proof of Concept**: Get editor compiling with new library structure
4. **Incremental**: Maintain working state throughout migration

### Success Criteria
- [ ] Library can be used independently of UI
- [ ] CLI tool successfully renders scenes
- [ ] Multiple backends can be swapped at runtime
- [ ] Post-processing pipeline is extensible
- [ ] Editor maintains all current functionality
- [ ] Clean separation of concerns achieved

This architecture will give you a solid foundation for future enhancements while maintaining clean modularity and extensibility.