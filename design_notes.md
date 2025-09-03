# Future Architecture Design

## Hardware Rendering Integration

### Option 1: Hybrid CPU/GPU Renderer
```cpp
class Renderer {
public:
    enum class Backend {
        CPU,           // Current software renderer
        OpenGL_Compute,
        Vulkan_Compute,
        CUDA,
        OptiX
    };
    
    void setBackend(Backend backend);
    void render(const Scene& scene, const Camera& camera, 
                RenderTarget& target, const RenderSettings& settings);
                
private:
    std::unique_ptr<RendererBackend> m_backend;
};

class RendererBackend {
public:
    virtual void render(...) = 0;
};

class CPURendererBackend : public RendererBackend { /* current code */ };
class GPURendererBackend : public RendererBackend { /* GPU compute shaders */ };
```

### Option 2: Separate GPU Renderer Class
```cpp
class CPURenderer { /* current renderer */ };
class GPURenderer { 
    // Uses OpenGL/Vulkan compute shaders
    // Renders directly to GPU texture
    // Much faster for real-time
};

// Editor chooses which renderer to use
```

## Editor Integration

### Multi-Viewport System
```cpp
class EditorApp {
    std::vector<Viewport> m_viewports;  // 3D view, UV editor, etc.
    std::unique_ptr<Renderer> m_renderer;
    
    struct Viewport {
        std::unique_ptr<RenderTarget> render_target;
        Camera camera;
        RenderSettings settings;  // Different quality per viewport
    };
};
```

### Asset Pipeline
```cpp
class AssetRenderer {
    // Generates thumbnails, previews
    // Uses same Renderer but different RenderTargets
    void generateThumbnail(const Scene& scene, const std::string& output_path);
    void generatePreview(const Material& material, RenderTarget& target);
};
```

## Texture Ownership Strategy

### Current: App Owns Everything
```
App
├── SDL_Texture (display)
├── m_viewport_data (CPU buffer)
└── PreviewRenderTarget (wrapper)
```

### Future: RenderTarget Owns Resources
```
GPURenderTarget
├── OpenGL_Texture_ID (GPU texture)
├── manages GPU memory
└── handles GPU/CPU sync

EditorViewport  
├── RenderTarget (CPU or GPU)
├── ImGui integration
└── viewport controls
```

## Hardware Rendering Considerations

### GPU Texture Flow:
```cpp
class OpenGLRenderTarget : public RenderTarget {
public:
    OpenGLRenderTarget(uint32_t width, uint32_t height);
    
    // Renderer writes to compute shader storage buffer
    void setPixel(uint32_t x, uint32_t y, const glm::vec3& color) override;
    
    // Get texture ID for ImGui::Image()
    uint32_t getTextureID() const { return m_texture_id; }
    
private:
    uint32_t m_texture_id;        // OpenGL texture
    uint32_t m_compute_buffer;    // Storage buffer for compute shader
};
```

### Benefits:
- Renderer stays the same API
- No CPU/GPU memory copying
- Much faster for real-time rendering
- Editor can display GPU textures directly

## Migration Path

### Phase 1 (Current): Software only
- ✅ Renderer + RenderTarget abstraction
- ✅ Clean separation of concerns

### Phase 2: Add GPU backend
- Add OpenGLRenderTarget
- Add compute shader path tracer
- Same Renderer API, different backend

### Phase 3: Editor integration  
- Multiple viewports using same renderer
- Asset preview system
- Material editor with real-time preview

### Phase 4: Advanced features
- Real-time denoising
- OptiX/CUDA acceleration
- Network rendering