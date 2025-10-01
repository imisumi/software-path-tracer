# Render Engine Scene Composition Design Document

## Overview

This document outlines a focused architecture for a render engine with scene composition capabilities. The design emphasizes model importing, positioning, basic shading, and viewport rendering - the core requirements for a production rendering system without animation, physics, or procedural generation complexity.

## Core Architecture

### Scene Management
- **Model Loading**: Import and manage 3D meshes from standard formats
- **Transform Hierarchy**: Position, rotate, and scale objects in 3D space
- **Material System**: Assign and manage surface properties
- **Lighting Setup**: Define scene illumination
- **Camera System**: Manage viewpoints for rendering

### Rendering Pipeline
- **Viewport Rendering**: Real-time preview with OpenGL/Vulkan
- **Production Rendering**: High-quality offline rendering
- **Material Evaluation**: Shader execution and texture sampling
- **Lighting Calculation**: Direct and indirect illumination

## Core Classes

### Base Scene Node

```cpp
using NodeID = uint32_t;

enum class NodeType {
    SCENE_ROOT,
    MESH_OBJECT,
    MATERIAL,
    LIGHT,
    CAMERA,
    GROUP
};

class SceneNode {
protected:
    static NodeID s_nextID;
    NodeID m_id;
    std::string m_name;
    NodeType m_type;
    
    // Hierarchy
    SceneNode* m_parent = nullptr;
    std::vector<std::unique_ptr<SceneNode>> m_children;
    
    // Transform
    Transform m_localTransform;
    mutable Transform m_worldTransform;
    mutable bool m_worldTransformDirty = true;
    
    // Visibility
    bool m_visible = true;
    bool m_castsShadows = true;
    bool m_receivesShadows = true;
    
public:
    SceneNode(NodeType type, const std::string& name = "Node");
    virtual ~SceneNode() = default;
    
    // Identity
    NodeID GetID() const { return m_id; }
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }
    NodeType GetType() const { return m_type; }
    
    // Hierarchy
    void AddChild(std::unique_ptr<SceneNode> child);
    void RemoveChild(NodeID childID);
    SceneNode* FindChild(NodeID id);
    SceneNode* GetParent() const { return m_parent; }
    const std::vector<std::unique_ptr<SceneNode>>& GetChildren() const { return m_children; }
    
    // Transform
    const Transform& GetLocalTransform() const { return m_localTransform; }
    const Transform& GetWorldTransform() const;
    void SetLocalTransform(const Transform& transform);
    void SetPosition(const Vector3& position);
    void SetRotation(const Vector3& rotation); // Euler angles
    void SetScale(const Vector3& scale);
    Vector3 GetPosition() const;
    Vector3 GetRotation() const;
    Vector3 GetScale() const;
    
    // Visibility
    bool IsVisible() const { return m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    bool CastsShadows() const { return m_castsShadows; }
    void SetCastsShadows(bool casts) { m_castsShadows = casts; }
    bool ReceivesShadows() const { return m_receivesShadows; }
    void SetReceivesShadows(bool receives) { m_receivesShadows = receives; }
    
    // Rendering interface
    virtual void CollectRenderData(RenderCollector& collector) {}
    virtual BoundingBox GetBoundingBox() const { return BoundingBox(); }
    
protected:
    void MarkWorldTransformDirty();
    void UpdateWorldTransform() const;
};

// Transform helper class
struct Transform {
    Vector3 position = Vector3::Zero();
    Quaternion rotation = Quaternion::Identity();
    Vector3 scale = Vector3::One();
    
    Matrix4 ToMatrix() const;
    static Transform FromMatrix(const Matrix4& matrix);
    Transform operator*(const Transform& other) const;
};
```

### Mesh Object Node

```cpp
class MeshObject : public SceneNode {
private:
    // Geometry Data
    std::shared_ptr<Mesh> m_mesh;
    BoundingBox m_boundingBox;
    
    // Material Assignments
    std::vector<MaterialAssignment> m_materialAssignments;
    MaterialNode* m_defaultMaterial = nullptr;
    
    // Render Data Cache
    mutable std::shared_ptr<RenderMesh> m_renderMesh;
    mutable bool m_renderMeshDirty = true;
    
public:
    MeshObject(const std::string& name = "Mesh");
    MeshObject(std::shared_ptr<Mesh> mesh, const std::string& name = "Mesh");
    
    // Mesh Management
    void SetMesh(std::shared_ptr<Mesh> mesh);
    std::shared_ptr<Mesh> GetMesh() const { return m_mesh; }
    
    // Material Management
    void SetMaterial(MaterialNode* material);
    void SetMaterial(MaterialNode* material, const std::string& selectionSet);
    MaterialNode* GetMaterial(int32_t submeshIndex = 0) const;
    const std::vector<MaterialAssignment>& GetMaterialAssignments() const;
    
    // Rendering
    void CollectRenderData(RenderCollector& collector) override;
    BoundingBox GetBoundingBox() const override;
    std::shared_ptr<RenderMesh> GetRenderMesh() const;
    
private:
    void UpdateBoundingBox();
    void UpdateRenderMesh() const;
};

// Material assignment for submeshes
struct MaterialAssignment {
    std::string selectionSet;  // Empty = default, or named selection
    MaterialNode* material;
    int32_t submeshIndex = -1; // -1 = all submeshes
    
    MaterialAssignment(MaterialNode* mat, const std::string& selection = "") 
        : material(mat), selectionSet(selection) {}
};
```

### Material System

```cpp
class MaterialNode : public SceneNode {
public:
    enum class MaterialType {
        STANDARD_PBR,
        EMISSION,
        GLASS,
        SUBSURFACE
    };
    
private:
    MaterialType m_materialType = MaterialType::STANDARD_PBR;
    
    // PBR Properties
    Color m_albedo = Color::White();
    float m_metallic = 0.0f;
    float m_roughness = 0.5f;
    float m_specular = 0.5f;
    float m_ior = 1.45f;
    Color m_emission = Color::Black();
    float m_emissionStrength = 0.0f;
    
    // Texture Maps
    std::shared_ptr<Texture> m_albedoMap;
    std::shared_ptr<Texture> m_normalMap;
    std::shared_ptr<Texture> m_roughnessMap;
    std::shared_ptr<Texture> m_metallicMap;
    std::shared_ptr<Texture> m_emissionMap;
    std::shared_ptr<Texture> m_aoMap;
    
    // Render Data
    mutable std::shared_ptr<MaterialData> m_materialData;
    mutable bool m_materialDirty = true;
    
public:
    MaterialNode(const std::string& name = "Material");
    
    // Material Type
    MaterialType GetMaterialType() const { return m_materialType; }
    void SetMaterialType(MaterialType type);
    
    // Basic Properties
    const Color& GetAlbedo() const { return m_albedo; }
    void SetAlbedo(const Color& albedo);
    
    float GetMetallic() const { return m_metallic; }
    void SetMetallic(float metallic);
    
    float GetRoughness() const { return m_roughness; }
    void SetRoughness(float roughness);
    
    float GetSpecular() const { return m_specular; }
    void SetSpecular(float specular);
    
    float GetIOR() const { return m_ior; }
    void SetIOR(float ior);
    
    const Color& GetEmission() const { return m_emission; }
    void SetEmission(const Color& emission);
    
    float GetEmissionStrength() const { return m_emissionStrength; }
    void SetEmissionStrength(float strength);
    
    // Texture Management
    void SetAlbedoMap(std::shared_ptr<Texture> texture);
    void SetNormalMap(std::shared_ptr<Texture> texture);
    void SetRoughnessMap(std::shared_ptr<Texture> texture);
    void SetMetallicMap(std::shared_ptr<Texture> texture);
    void SetEmissionMap(std::shared_ptr<Texture> texture);
    void SetAOMap(std::shared_ptr<Texture> texture);
    
    std::shared_ptr<Texture> GetAlbedoMap() const { return m_albedoMap; }
    std::shared_ptr<Texture> GetNormalMap() const { return m_normalMap; }
    std::shared_ptr<Texture> GetRoughnessMap() const { return m_roughnessMap; }
    std::shared_ptr<Texture> GetMetallicMap() const { return m_metallicMap; }
    std::shared_ptr<Texture> GetEmissionMap() const { return m_emissionMap; }
    std::shared_ptr<Texture> GetAOMap() const { return m_aoMap; }
    
    // Render Data
    std::shared_ptr<MaterialData> GetMaterialData() const;
    
private:
    void MarkMaterialDirty() { m_materialDirty = true; }
    void UpdateMaterialData() const;
};
```

### Lighting System

```cpp
class LightNode : public SceneNode {
public:
    enum class LightType {
        DIRECTIONAL,    // Sun/distant light
        POINT,          // Omni light
        SPOT,           // Spotlight
        AREA_RECT,      // Rectangular area light
        AREA_SPHERE,    // Spherical area light
        HDRI            // Environment lighting
    };
    
private:
    LightType m_lightType = LightType::POINT;
    Color m_color = Color::White();
    float m_intensity = 1.0f;
    float m_temperature = 6500.0f; // Kelvin
    bool m_useTemperature = false;
    
    // Attenuation
    float m_range = 10.0f;
    float m_falloffExponent = 2.0f; // Inverse square by default
    
    // Spot light properties
    float m_spotAngle = 45.0f;      // degrees
    float m_spotFalloff = 0.0f;     // edge softness
    
    // Area light properties
    Vector2 m_areaSize = Vector2(1.0f, 1.0f);
    
    // Shadow properties
    bool m_castShadows = true;
    float m_shadowBias = 0.0001f;
    int32_t m_shadowMapSize = 1024;
    
    // HDRI properties
    std::shared_ptr<Texture> m_hdriTexture;
    float m_hdriRotation = 0.0f;
    
public:
    LightNode(const std::string& name = "Light");
    
    // Light Type
    LightType GetLightType() const { return m_lightType; }
    void SetLightType(LightType type);
    
    // Color and Intensity
    const Color& GetColor() const;
    void SetColor(const Color& color);
    
    float GetIntensity() const { return m_intensity; }
    void SetIntensity(float intensity);
    
    float GetTemperature() const { return m_temperature; }
    void SetTemperature(float kelvin);
    
    bool UsesTemperature() const { return m_useTemperature; }
    void SetUseTemperature(bool use);
    
    // Attenuation
    float GetRange() const { return m_range; }
    void SetRange(float range);
    
    float GetFalloffExponent() const { return m_falloffExponent; }
    void SetFalloffExponent(float exponent);
    
    // Spot Light
    float GetSpotAngle() const { return m_spotAngle; }
    void SetSpotAngle(float degrees);
    
    float GetSpotFalloff() const { return m_spotFalloff; }
    void SetSpotFalloff(float falloff);
    
    // Area Light
    const Vector2& GetAreaSize() const { return m_areaSize; }
    void SetAreaSize(const Vector2& size);
    
    // Shadows
    bool CastsShadows() const { return m_castShadows; }
    void SetCastsShadows(bool casts);
    
    float GetShadowBias() const { return m_shadowBias; }
    void SetShadowBias(float bias);
    
    int32_t GetShadowMapSize() const { return m_shadowMapSize; }
    void SetShadowMapSize(int32_t size);
    
    // HDRI
    void SetHDRI(std::shared_ptr<Texture> hdri);
    std::shared_ptr<Texture> GetHDRI() const { return m_hdriTexture; }
    
    float GetHDRIRotation() const { return m_hdriRotation; }
    void SetHDRIRotation(float degrees);
    
    // Rendering
    void CollectRenderData(RenderCollector& collector) override;
    LightData GetLightData() const;
};
```

### Camera System

```cpp
class CameraNode : public SceneNode {
public:
    enum class ProjectionType {
        PERSPECTIVE,
        ORTHOGRAPHIC
    };
    
private:
    ProjectionType m_projectionType = ProjectionType::PERSPECTIVE;
    
    // Perspective properties
    float m_fov = 50.0f;            // Field of view in degrees
    float m_focalLength = 50.0f;    // mm (alternative to FOV)
    float m_sensorWidth = 36.0f;    // mm
    bool m_useFocalLength = false;  // Use focal length instead of FOV
    
    // Clipping planes
    float m_nearClip = 0.1f;
    float m_farClip = 1000.0f;
    
    // Orthographic properties
    float m_orthoSize = 5.0f;       // Half-height of ortho view
    
    // Depth of field
    bool m_enableDOF = false;
    float m_focusDistance = 10.0f;
    float m_aperture = 2.8f;        // f-stop
    
public:
    CameraNode(const std::string& name = "Camera");
    
    // Projection
    ProjectionType GetProjectionType() const { return m_projectionType; }
    void SetProjectionType(ProjectionType type);
    
    // Field of View
    float GetFOV() const { return m_fov; }
    void SetFOV(float degrees);
    
    float GetFocalLength() const { return m_focalLength; }
    void SetFocalLength(float mm);
    
    float GetSensorWidth() const { return m_sensorWidth; }
    void SetSensorWidth(float mm);
    
    bool UsesFocalLength() const { return m_useFocalLength; }
    void SetUseFocalLength(bool use);
    
    // Clipping
    float GetNearClip() const { return m_nearClip; }
    void SetNearClip(float distance);
    
    float GetFarClip() const { return m_farClip; }
    void SetFarClip(float distance);
    
    // Orthographic
    float GetOrthoSize() const { return m_orthoSize; }
    void SetOrthoSize(float size);
    
    // Depth of Field
    bool IsDOFEnabled() const { return m_enableDOF; }
    void SetDOFEnabled(bool enabled);
    
    float GetFocusDistance() const { return m_focusDistance; }
    void SetFocusDistance(float distance);
    
    float GetAperture() const { return m_aperture; }
    void SetAperture(float fstop);
    
    // Camera Matrices
    Matrix4 GetViewMatrix() const;
    Matrix4 GetProjectionMatrix(float aspectRatio) const;
    Matrix4 GetViewProjectionMatrix(float aspectRatio) const;
    
    // Camera Data
    CameraData GetCameraData(float aspectRatio) const;
    
    // Utility
    Ray ScreenPointToRay(const Vector2& screenPoint, const Vector2& screenSize) const;
    Vector3 WorldToScreen(const Vector3& worldPoint, const Vector2& screenSize) const;
};
```

### Scene Document

```cpp
class Scene {
private:
    // Scene hierarchy
    std::unique_ptr<SceneNode> m_rootNode;
    std::unordered_map<NodeID, SceneNode*> m_nodeRegistry;
    
    // Asset management
    std::unique_ptr<AssetManager> m_assetManager;
    
    // Active camera
    CameraNode* m_activeCamera = nullptr;
    
    // Environment
    Color m_backgroundColor = Color(0.2f, 0.2f, 0.2f);
    LightNode* m_environmentLight = nullptr;
    
public:
    Scene();
    ~Scene();
    
    // Node Management
    SceneNode* GetRootNode() const { return m_rootNode.get(); }
    
    template<typename T, typename... Args>
    T* CreateNode(Args&&... args);
    
    bool DeleteNode(NodeID id);
    SceneNode* FindNode(NodeID id);
    SceneNode* FindNode(const std::string& name);
    
    // Model Loading
    MeshObject* LoadModel(const std::string& filePath, const std::string& name = "");
    
    // Material Management
    MaterialNode* CreateMaterial(const std::string& name = "Material");
    
    // Lighting
    LightNode* CreateLight(LightNode::LightType type, const std::string& name = "Light");
    void SetEnvironmentLight(LightNode* light);
    LightNode* GetEnvironmentLight() const { return m_environmentLight; }
    
    // Camera
    CameraNode* CreateCamera(const std::string& name = "Camera");
    void SetActiveCamera(CameraNode* camera);
    CameraNode* GetActiveCamera() const { return m_activeCamera; }
    
    // Environment
    const Color& GetBackgroundColor() const { return m_backgroundColor; }
    void SetBackgroundColor(const Color& color);
    
    // Asset Management
    AssetManager* GetAssetManager() const { return m_assetManager.get(); }
    
    // Rendering
    void CollectRenderData(RenderCollector& collector);
    BoundingBox GetSceneBounds() const;
    
    // Traversal
    void TraverseNodes(std::function<bool(SceneNode*)> visitor);
    void TraverseNodes(SceneNode* root, std::function<bool(SceneNode*)> visitor);
    
private:
    void RegisterNode(SceneNode* node);
    void UnregisterNode(NodeID id);
    void CollectNodeRenderData(SceneNode* node, RenderCollector& collector);
};
```

## Asset Management

```cpp
class AssetManager {
private:
    // Loaded assets
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_meshes;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_textures;
    
    // Asset loaders
    std::unique_ptr<MeshLoader> m_meshLoader;
    std::unique_ptr<TextureLoader> m_textureLoader;
    
public:
    AssetManager();
    
    // Mesh Loading
    std::shared_ptr<Mesh> LoadMesh(const std::string& filePath);
    std::shared_ptr<Mesh> GetMesh(const std::string& filePath);
    
    // Texture Loading  
    std::shared_ptr<Texture> LoadTexture(const std::string& filePath);
    std::shared_ptr<Texture> GetTexture(const std::string& filePath);
    
    // Asset Management
    void UnloadAsset(const std::string& filePath);
    void UnloadAllAssets();
    size_t GetMemoryUsage() const;
    
private:
    std::string NormalizePath(const std::string& path);
};
```

## Usage Example

```cpp
int main() {
    // Create scene
    Scene scene;
    
    // Load a model
    auto* meshObj = scene.LoadModel("assets/models/teapot.obj", "Teapot");
    meshObj->SetPosition(Vector3(0, 0, 0));
    meshObj->SetRotation(Vector3(0, 45, 0));
    
    // Create material
    auto* material = scene.CreateMaterial("TeapotMaterial");
    material->SetAlbedo(Color(0.8f, 0.3f, 0.2f));
    material->SetMetallic(0.0f);
    material->SetRoughness(0.3f);
    
    // Assign material
    meshObj->SetMaterial(material);
    
    // Add lighting
    auto* sunLight = scene.CreateLight(LightNode::LightType::DIRECTIONAL, "Sun");
    sunLight->SetPosition(Vector3(5, 10, 5));
    sunLight->SetRotation(Vector3(-45, 30, 0));
    sunLight->SetColor(Color(1.0f, 0.95f, 0.8f));
    sunLight->SetIntensity(3.0f);
    
    // Setup camera
    auto* camera = scene.CreateCamera("MainCamera");
    camera->SetPosition(Vector3(0, 2, 8));
    camera->SetRotation(Vector3(-15, 0, 0));
    scene.SetActiveCamera(camera);
    
    // Render
    RenderSettings settings;
    settings.width = 1920;
    settings.height = 1080;
    settings.samples = 64;
    
    RenderEngine renderer;
    auto result = renderer.Render(scene, settings);
    
    return 0;
}
```

## Implementation Priority

### Phase 1: Core Structure
1. SceneNode base class with transform hierarchy
2. MeshObject for loading and positioning models
3. Basic Scene container
4. Simple AssetManager for mesh loading

### Phase 2: Material System  
1. MaterialNode with PBR properties
2. Texture loading and management
3. Material assignment to mesh objects

### Phase 3: Lighting and Camera
1. LightNode with basic light types
2. CameraNode with perspective/orthographic projection
3. Environment lighting support

### Phase 4: Rendering Integration
1. RenderCollector for gathering scene data
2. Viewport rendering integration
3. Production rendering pipeline

This focused design gives you a solid foundation for model importing, positioning, and shading without the complexity of animation, physics, or procedural generation.