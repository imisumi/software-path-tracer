# Metallic Materials Guide

Your path tracer now supports **PBR (Physically Based Rendering) metallic materials** using the Cook-Torrance microfacet BRDF with GGX distribution!

## Quick Start

### Creating a Diffuse Material (default)

```cpp
MaterialDescriptor diffuse;
diffuse.type = MaterialType::DIFFUSE;
diffuse.parameters["albedo"] = glm::vec3(0.8f, 0.2f, 0.2f);  // Red

scene->setMaterial(sphere->GetID(), diffuse);
```

### Creating a Metallic Material

```cpp
MaterialDescriptor metal;
metal.type = MaterialType::METALLIC;
metal.parameters["albedo"] = glm::vec3(1.0f, 0.84f, 0.0f);  // Gold color
metal.parameters["roughness"] = 0.1f;  // 0.0 = mirror, 1.0 = rough
metal.parameters["metallic"] = 1.0f;   // 0.0 = dielectric, 1.0 = metal

scene->setMaterial(sphere->GetID(), metal);
```

## Material Parameters

### **albedo** (glm::vec3)
- Base color of the material
- For metals: This is the reflectance color (e.g., gold = yellowish)
- For dielectrics: This is the diffuse color
- Default: `glm::vec3(0.7f)` (grey)

### **roughness** (float, 0.0 to 1.0)
- Surface microsurface roughness
- `0.0` = Perfect mirror (sharp reflections)
- `0.5` = Brushed metal
- `1.0` = Completely rough (diffuse-like)
- Default: `1.0f`

### **metallic** (float, 0.0 to 1.0)
- How metallic the surface is
- `0.0` = Dielectric (plastic, wood, etc.)
- `1.0` = Full metal (conducts electricity)
- `0.5` = Mix of both (not physically accurate, but useful)
- Default: `0.0f`

## Example Materials

### Mirror (Perfect Reflection)
```cpp
MaterialDescriptor mirror;
mirror.type = MaterialType::METALLIC;
mirror.parameters["albedo"] = glm::vec3(1.0f);
mirror.parameters["roughness"] = 0.0f;
mirror.parameters["metallic"] = 1.0f;
```

### Brushed Aluminum
```cpp
MaterialDescriptor aluminum;
aluminum.type = MaterialType::METALLIC;
aluminum.parameters["albedo"] = glm::vec3(0.91f, 0.92f, 0.92f);
aluminum.parameters["roughness"] = 0.3f;
aluminum.parameters["metallic"] = 1.0f;
```

### Gold
```cpp
MaterialDescriptor gold;
gold.type = MaterialType::METALLIC;
gold.parameters["albedo"] = glm::vec3(1.0f, 0.84f, 0.0f);
gold.parameters["roughness"] = 0.1f;
gold.parameters["metallic"] = 1.0f;
```

### Copper
```cpp
MaterialDescriptor copper;
copper.type = MaterialType::METALLIC;
copper.parameters["albedo"] = glm::vec3(0.95f, 0.64f, 0.54f);
copper.parameters["roughness"] = 0.2f;
copper.parameters["metallic"] = 1.0f;
```

### Rough Iron
```cpp
MaterialDescriptor iron;
iron.type = MaterialType::METALLIC;
iron.parameters["albedo"] = glm::vec3(0.56f, 0.57f, 0.58f);
iron.parameters["roughness"] = 0.6f;
iron.parameters["metallic"] = 1.0f;
```

### Plastic (Dielectric)
```cpp
MaterialDescriptor plastic;
plastic.type = MaterialType::METALLIC;
plastic.parameters["albedo"] = glm::vec3(0.8f, 0.1f, 0.1f);  // Red plastic
plastic.parameters["roughness"] = 0.4f;
plastic.parameters["metallic"] = 0.0f;  // NOT metal
```

## How It Works

### BRDF Components

The metallic material uses the **Cook-Torrance microfacet BRDF**:

```
f(wi, wo) = diffuse + specular

where:
  diffuse = (1 - F) * (1 - metallic) * albedo / π
  specular = (D * G * F) / (4 * ndotv * ndotl)

  D = GGX Normal Distribution (controls highlight shape)
  G = Smith's Geometry Function (shadowing/masking)
  F = Fresnel (edge reflection, using Schlick approximation)
```

### Importance Sampling

Rays are sampled using **GGX importance sampling**:
- Samples microfacet normals proportional to the GGX distribution
- Reflects view direction around sampled normal
- Much more efficient than uniform hemisphere sampling
- Dramatically reduces noise for glossy surfaces

### Energy Conservation

The BRDF is **energy conserving**:
- Fresnel term F determines how much light reflects
- Remaining energy (1 - F) goes to diffuse
- Metallic parameter blends between metal (no diffuse) and dielectric (has diffuse)

## Visual Results

### Roughness Variation
- **roughness = 0.0**: Sharp mirror reflections of sky
- **roughness = 0.2**: Slightly blurred reflections
- **roughness = 0.5**: Glossy appearance
- **roughness = 0.8**: Very blurry reflections
- **roughness = 1.0**: Almost diffuse (but still has some specular)

### Metallic Variation
- **metallic = 0.0**: Plastic-like, has diffuse + weak specular
- **metallic = 0.5**: Blends diffuse and metallic (not physically accurate)
- **metallic = 1.0**: Pure metal, no diffuse, strong colored reflections

## Tips for Best Results

1. **Metals reflect your skybox** - Make sure your sky has interesting colors/gradients
2. **Rough metals need more samples** - Increase samples per pixel for smoother results
3. **Mix materials** - Create a scene with different roughness values to see the full range
4. **Use realistic metal colors** - Look up real metal albedo values for physically accurate renders

## Common Issues

### Black/Dark Spheres
- Check that roughness > 0.0 (pure mirrors can look black without strong light sources)
- Ensure sky is bright enough
- Try adding more bounces (`MAX_BOUNCES`)

### Noisy Reflections
- Increase samples per pixel
- Lower roughness values converge faster
- Higher roughness needs more samples

### No Visible Difference from Diffuse
- Set `metallic = 1.0` to see full metallic behavior
- Lower `roughness` to see clear reflections
- Make sure material type is set to `MaterialType::METALLIC`

## Next Steps

- Add point lights for sharper highlights
- Add HDR environment maps for realistic lighting
- Add glass/transmission materials (refraction)
- Implement Multiple Importance Sampling for better convergence
