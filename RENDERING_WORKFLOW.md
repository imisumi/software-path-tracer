# Path Tracing Rendering Workflow

This document explains the professional path tracing workflow implemented in CPUPathTracer.

## Overview: The Rendering Equation

```
Lo(x, wo) = Le(x, wo) + ∫ f(x, wi, wo) * Li(x, wi) * cos(θ) dω
```

- **Lo**: Outgoing radiance (light leaving the surface toward the camera)
- **Le**: Emitted radiance (for light sources)
- **f**: BSDF (Bidirectional Scattering Distribution Function)
- **Li**: Incoming radiance (light arriving from direction wi)
- **cos(θ)**: Angle between surface normal and incoming light
- **∫ dω**: Integral over the hemisphere

## Monte Carlo Estimation

We can't evaluate the integral analytically, so we use Monte Carlo sampling:

```
Lo ≈ Le + (f * Li * cos(θ)) / pdf
```

Where **pdf** is the probability density of sampling direction wi.

## Rendering Pipeline

### 1. **Main Render Loop** (`render()`)
- Loops over all pixels
- Generates camera rays
- Traces rays through the scene
- Accumulates samples over multiple frames

### 2. **Camera Ray Generation** (`generate_camera_ray()`)
- Converts pixel coordinates to world-space ray direction
- Handles aspect ratio and field of view
- Future: Add depth of field, lens effects

### 3. **Path Tracing** (`trace_ray()`)

#### Step-by-step workflow:

1. **Intersect Scene** (`intersect_scene()`)
   - Cast ray into scene using Embree
   - Returns hit information or miss

2. **Get Material Properties** (`get_albedo()`)
   - Retrieves material from scene node
   - Currently: albedo only
   - Future: roughness, metallic, emission, etc.

3. **Direct Lighting** (`sample_direct_lighting()`)
   - **Next Event Estimation (NEE)**
   - Sample lights directly for lower variance
   - Currently: Returns zero (no lights)
   - Future: Point lights, area lights, HDR environment

4. **Sample BSDF** (`sample_bsdf()`)
   - Generate random direction for indirect lighting
   - Currently: Cosine-weighted hemisphere (importance sampling)
   - Returns: direction `wi` and probability `pdf`
   - Future: GGX for metallic, Fresnel for glass

5. **Evaluate BSDF** (`evaluate_bsdf()`)
   - Calculate how much light reflects from `wi` to `wo`
   - Currently: Lambertian BRDF = `albedo / π`
   - Future: Cook-Torrance microfacet, Disney BSDF

6. **Calculate PDF** (`pdf_bsdf()`)
   - Probability of sampling the chosen direction
   - Currently: `cos(θ) / π` for cosine-weighted
   - Future: GGX distribution PDF, mixture PDFs

7. **Update Throughput**
   ```cpp
   throughput *= (bsdf * cos(θ)) / pdf
   ```
   - For Lambertian: `(albedo/π * cos(θ)) / (cos(θ)/π) = albedo`
   - The math simplifies, but we compute explicitly for extensibility

8. **Russian Roulette**
   - Randomly terminate paths to limit bounces
   - Probability based on throughput energy
   - Unbiased estimator

9. **Recurse**
   - Setup next ray from hit point
   - Continue until max bounces or termination

## BSDF Functions Explained

### `evaluate_bsdf(wi, wo, normal, albedo)`
**What it does:** Returns the BRDF value f(wi, wo)

**Current implementation:** Lambertian (diffuse)
```cpp
f = albedo / π
```

**Future materials:**
- **Metallic:** Cook-Torrance microfacet BRDF
- **Glass:** Fresnel-modulated refraction/reflection
- **Emissive:** Return emission color

### `sample_bsdf(wo, normal, rng_state, pdf_out)`
**What it does:** Samples a random incoming direction wi

**Current implementation:** Cosine-weighted hemisphere
```cpp
// Generate wi proportional to cos(θ)
pdf = cos(θ) / π
```

**Why cosine-weighted?**
- Importance sampling for Lambertian surfaces
- Reduces variance by sampling more likely directions

**Future materials:**
- **Metallic:** Sample GGX microfacet distribution
- **Glass:** Choose reflect/refract based on Fresnel
- **Mixture:** Randomly choose BSDF lobe

### `pdf_bsdf(wi, wo, normal)`
**What it does:** Returns probability of sampling direction wi

**Current implementation:**
```cpp
pdf = cos(θ) / π
```

**Why separate from sampling?**
- Needed for Multiple Importance Sampling (MIS)
- When combining light sampling + BSDF sampling

### `sample_direct_lighting(hit_pos, normal, albedo, rng_state)`
**What it does:** Explicitly sample lights for direct illumination

**Current implementation:** Returns zero (no lights yet)

**Future implementation:**
```cpp
1. Choose a random light
2. Sample a point on the light
3. Cast shadow ray to check visibility
4. Evaluate BSDF * light contribution / pdf
5. Combine with BSDF sampling using MIS
```

## Current Material Support

✅ **Lambertian (Diffuse)**
- Cosine-weighted hemisphere sampling
- BRDF: albedo / π
- PDF: cos(θ) / π
- Result: albedo (PDF cancels with cos term)

## Future Material Extensions

To add new materials, modify these functions:

1. **`evaluate_bsdf()`** - Add BRDF evaluation
2. **`sample_bsdf()`** - Add importance sampling strategy
3. **`pdf_bsdf()`** - Add PDF calculation
4. **Material descriptor** - Add new parameters (roughness, metallic, IOR)

### Example: Adding Metallic Material

```cpp
glm::vec3 evaluate_bsdf(...) {
    if (material.type == METALLIC) {
        // Cook-Torrance BRDF
        float D = GGX_Distribution(roughness, ...);
        float G = SmithGGX_Geometry(...);
        vec3 F = Fresnel_Schlick(metallic, ...);
        return (D * G * F) / (4 * cos_wo * cos_wi);
    }
    // ... existing Lambertian code
}

glm::vec3 sample_bsdf(...) {
    if (material.type == METALLIC) {
        // Sample GGX distribution
        vec3 half_vector = Sample_GGX(roughness, ...);
        wi = reflect(-wo, half_vector);
        pdf_out = GGX_PDF(...);
        return wi;
    }
    // ... existing cosine-weighted code
}
```

## Key Differences from Simple Path Tracer

| Aspect | Simple | Professional |
|--------|--------|-------------|
| BRDF | Implicit (baked into sampling) | Explicit `evaluate_bsdf()` |
| PDF | Hidden in code | Explicit `pdf_bsdf()` |
| Light sampling | Only indirect (bounces) | Direct + Indirect |
| Material system | Hardcoded | Extensible via BSDF interface |
| Multiple materials | Requires rewrite | Just add new BSDF functions |

## Benefits of This Structure

1. **Extensible** - Add new materials without touching core path tracing
2. **Correct** - Explicitly shows Monte Carlo math
3. **Educational** - Clear separation of BSDF, PDF, sampling
4. **Professional** - Matches industry rendering code structure
5. **MIS-ready** - Can combine light + BSDF sampling later

## Next Steps

1. ✅ Structure in place (placeholders work)
2. 🔲 Add point lights to `sample_direct_lighting()`
3. 🔲 Add metallic materials (GGX BRDF)
4. 🔲 Add glass materials (Fresnel + refraction)
5. 🔲 Add emissive materials (area lights)
6. 🔲 Implement Multiple Importance Sampling (MIS)
7. 🔲 Add HDR environment map sampling
