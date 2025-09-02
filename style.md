# C++ Naming Convention Style Guide

## Core Principle
**Be consistent within your project.** Pick one style and stick with it throughout.

## Recommended Style (Modern C++)

### Variables

**Local variables & function parameters**: `snake_case`
```cpp
void intersect_ray(const Ray& ray, float max_distance) {
    float closest_hit = FLT_MAX;
    bool found_intersection = false;
    glm::vec3 hit_point = ray.origin;
    
    for (size_t sphere_index = 0; sphere_index < spheres.size(); ++sphere_index) {
        // ...
    }
}
```

**Private member variables**: `m_snake_case` (m_ prefix)
```cpp
class Scene {
private:
    std::vector<glm::vec3> m_sphere_centers;
    std::vector<float> m_sphere_radii;
    MaterialData m_default_material;
    BVH m_acceleration_structure;
};
```

**Public member variables** (structs): `snake_case` (no prefix)
```cpp
struct SphereData {
    std::vector<glm::vec3> centers;           // Public, no prefix
    std::vector<float> radii;
    std::vector<uint32_t> material_indices;
};

struct HitInfo {
    float t;
    glm::vec3 point;
    glm::vec3 normal;
    uint32_t material_id;
};
```

### Functions and Methods

**Functions and methods**: `snake_case`
```cpp
bool intersect_sphere(const Ray& ray, glm::vec3 center, float radius);
void add_sphere(glm::vec3 center, float radius, uint32_t material_id);
Color trace_path(const Ray& ray, int depth);
glm::vec3 sample_hemisphere(float u1, float u2);
```

### Types

**Classes, structs, enums**: `PascalCase`
```cpp
class PathTracer {
    // ...
};

struct MaterialData {
    // ...
};

enum class MaterialType {
    Lambertian,
    Metal,
    Dielectric
};

using Color = glm::vec3;  // Type aliases also PascalCase
```

### Constants and Enumerators

**Constants**: `SCREAMING_SNAKE_CASE`
```cpp
constexpr float PI = 3.14159265359f;
constexpr float EPSILON = 1e-6f;
constexpr int MAX_DEPTH = 50;
```

**Enum values**: `PascalCase` (in scoped enums)
```cpp
enum class MaterialType {
    Lambertian,
    Metal,
    Dielectric,
    Emissive
};
```

### Namespaces and Files

**Namespaces**: `snake_case`
```cpp
namespace raytracer {
namespace math_utils {
    float clamp(float value, float min_val, float max_val);
}
}
```

**Files**: `snake_case`
```
sphere_data.h
sphere_data.cpp
path_tracer.h
path_tracer.cpp
material_data.h
bvh_node.h
```

## Complete Example

```cpp
// sphere_data.h
#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace raytracer {

struct SphereData {
    // Public members - no prefix
    std::vector<glm::vec3> centers;
    std::vector<float> radii;
    std::vector<uint32_t> material_indices;
    
    // Methods
    void add_sphere(glm::vec3 center, float radius, uint32_t material_index);
    void remove_sphere(size_t index);
    size_t size() const { return centers.size(); }
    void reserve(size_t count);
};

class Scene {
public:
    void add_sphere(glm::vec3 center, float radius, uint32_t material_id);
    bool intersect(const Ray& ray, HitInfo& hit_info) const;
    
private:
    // Private members - m_ prefix
    SphereData m_spheres;
    std::vector<MaterialData> m_materials;
    BVH m_bvh_tree;
};

// Free functions
bool intersect_sphere(const Ray& ray, glm::vec3 center, float radius, HitInfo& hit);
glm::vec3 random_in_unit_sphere(RandomGenerator& rng);

// Constants
constexpr float MAX_RAY_DISTANCE = 1000.0f;
constexpr int DEFAULT_SAMPLE_COUNT = 100;

} // namespace raytracer
```

## Alternative Styles (Choose One)

### Google Style (camelCase for variables)
```cpp
void intersectRay(const Ray& ray, float maxDistance) {
    float closestHit = FLT_MAX;
    bool foundIntersection = false;
    
private:
    std::vector<glm::vec3> sphereCenters_;  // Trailing underscore
    std::vector<float> sphereRadii_;
}
```

### Microsoft Style (PascalCase for public members)
```cpp
struct SphereData {
    std::vector<glm::vec3> Centers;         // PascalCase public
    std::vector<float> Radii;
    
private:
    int m_sphereCount;                      // Hungarian-style private
};
```

## Recommendations by Category

### Performance-Critical Code (Rendering)
- **Favor shorter names** in hot loops: `i`, `j`, `t`, `hit`
- **Use descriptive names** for setup/initialization code
```cpp
// Hot loop - short names OK
for (size_t i = 0; i < spheres.size(); ++i) {
    float t = intersect_sphere(ray, spheres.centers[i], spheres.radii[i]);
    if (t > 0 && t < closest_t) {
        closest_t = t;
        hit_index = i;
    }
}

// Setup code - descriptive names
void build_acceleration_structure() {
    std::vector<BVHNode> leaf_nodes;
    std::vector<AABB> bounding_boxes;
    // ...
}
```

### Data Structures
- **Use struct for plain data** with public members
- **Use class for behavior** with private members
```cpp
// Plain data - struct with public access
struct Triangle {
    glm::vec3 v0, v1, v2;
    glm::vec3 normal;
    uint32_t material_id;
};

// Behavior - class with encapsulation
class PathTracer {
public:
    Color trace_ray(const Ray& ray, int depth);
private:
    Scene m_scene;
    RandomGenerator m_rng;
};
```

## Tools for Enforcement

### clang-format Configuration
```yaml
# .clang-format
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
```

### Naming Conventions Summary Table

| Element | Style | Example |
|---------|-------|---------|
| Local variables | `snake_case` | `hit_distance` |
| Function parameters | `snake_case` | `ray_direction` |
| Private members | `m_snake_case` | `m_sphere_data` |
| Public members | `snake_case` | `material_id` |
| Functions/methods | `snake_case` | `intersect_ray` |
| Classes/structs | `PascalCase` | `PathTracer` |
| Constants | `SCREAMING_SNAKE_CASE` | `MAX_DEPTH` |
| Files | `snake_case` | `path_tracer.cpp` |

## Final Advice

1. **Pick one style and be consistent**
2. **Prioritize readability over brevity** (except in hot loops)
3. **Use meaningful names** - `material_id` over `mid`
4. **Avoid Hungarian notation** - the compiler knows the types
5. **Consider your team's preferences** if working with others

The recommended style above follows modern C++ guidelines and works well for performance-critical graphics code.