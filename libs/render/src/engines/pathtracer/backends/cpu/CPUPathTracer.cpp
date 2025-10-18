#include "CPUPathTracer.h"

#include <embree4/rtcore.h>

#include <algorithm>  // For std::clamp
#include <cassert>
#include <execution> // For parallel execution policies
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <numeric>    // For std::iota
#include <ranges>

#include "render/Color.h"
#include "render/Log.h"
#include "render/Scene.h"
#include "render/Types.h"
#include "render_assert.h"

namespace render
{

CPUPathTracer::CPUPathTracer()
{
    // Initialize Embree device and setup ray tracing acceleration structures
    // This will contain the logic currently in EmbreeRenderTarget constructor

    render::Log::info("Initializing CPU Path Tracer with Embree backend...");

    m_renderSettings = std::make_shared<RenderSettings>();
    initialize_embree();
}

CPUPathTracer::~CPUPathTracer()
{
    // Cleanup Embree resources
    // This will contain the logic currently in EmbreeRenderTarget destructor
}

glm::vec3 CPUPathTracer::generate_camera_ray(uint32_t x, uint32_t y, uint32_t width,
                                             uint32_t height) const
{
    const float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);
    const float inv_width = 1.0f / width;
    const float inv_height = 1.0f / height;

    float u = x * inv_width;
    float v = 1.0f - y * inv_height;
    float uv_x = (u * 2.0f - 1.0f) * aspect_ratio;
    float uv_y = v * 2.0f - 1.0f;

    float len = sqrtf(uv_x * uv_x + uv_y * uv_y + 1.0f);
    return glm::vec3(uv_x / len, uv_y / len, 1.0f / len);
}


void CPUPathTracer::render()
{
    verify(m_embreeDevice && m_embreeScene, "Embree not initialized");
    verify(m_scene != nullptr, "Scene not set before rendering");
    invalidate();

    const uint32_t width = m_render_result.width;
    const uint32_t height = m_render_result.height;

    // Create tiles if needed
    if (m_tiles.empty())
    {
        create_tiles(width, height);
    }

    // Camera position: Far back looking down +Z, above the ground
    const glm::vec3 ray_origin(0.0f, 2.0f, -15.0f);

    // Render tiles in parallel
    std::for_each(std::execution::par_unseq, m_tiles.begin(), m_tiles.end(),
        [&, ray_origin](const RenderTile& tile)
        {
            // Generate random color per tile (using tile coordinates as seed)
            uint32_t tile_seed = tile.x_start + tile.y_start * width + m_frameCount * RNG_PRIME;

            // Render all pixels in this tile
            for (uint32_t y = tile.y_start; y < tile.y_end; y++)
            {
                for (uint32_t x = tile.x_start; x < tile.x_end; x++)
                {
                    uint32_t rng_state = get_rng_state(width, height, x, y, m_frameCount + 1);
                    glm::vec3 ray_direction = generate_camera_ray(x, y, width, height);

                    // For now, use random tile color instead of trace_ray
                    // glm::vec4 color = tile_color;
                    glm::vec4 color = trace_ray(ray_origin, ray_direction, rng_state);

                    uint32_t idx = 4 * (y * width + x);
                    m_accumulation_buffer[idx + 0] += color.r;
                    m_accumulation_buffer[idx + 1] += color.g;
                    m_accumulation_buffer[idx + 2] += color.b;
                    m_accumulation_buffer[idx + 3] += color.a;
                }
            }
        });

    m_frameCount++;
}

const PathTracer::RenderResult& CPUPathTracer::get_render_result()
{
    assert(m_frameCount > 0 && "No frames rendered yet");

    // Generate pixel indices for parallel processing
    const uint32_t total_pixels = m_render_result.width * m_render_result.height;
    std::vector<uint32_t> pixel_indices(total_pixels);
    std::iota(pixel_indices.begin(), pixel_indices.end(), 0);

    // Convert accumulation buffer to 8-bit RGBA for output (parallel)
    std::for_each(std::execution::par, pixel_indices.begin(), pixel_indices.end(),
        [this](uint32_t idx)
        {
            float r = m_accumulation_buffer[4 * idx + 0] / (float)m_frameCount;
            float g = m_accumulation_buffer[4 * idx + 1] / (float)m_frameCount;
            float b = m_accumulation_buffer[4 * idx + 2] / (float)m_frameCount;
            float a = m_accumulation_buffer[4 * idx + 3] / (float)m_frameCount;

            // Clamp to [0,1]
            r = std::clamp(r, 0.0f, 1.0f);
            g = std::clamp(g, 0.0f, 1.0f);
            b = std::clamp(b, 0.0f, 1.0f);
            a = std::clamp(a, 0.0f, 1.0f);

            m_render_result.image_buffer[idx] =
                rgba_to_uint32((uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f),
                               (uint8_t)(a * 255.0f));
        });

    return m_render_result;
}

void CPUPathTracer::invalidate()
{
    bool needs_rebuild = false;
    if (m_scene->hasChanges())
    {
        // TODO: update embree
        //  bitmask for different changes, some require embree rebuild some dont
        m_frameCount = 0;
        m_outputDirty = true;

        // temporary - always rebuild for now
        needs_rebuild = true;
    }
    if (m_renderSettings->isDirty())
    {
        m_frameCount = 0;
        m_outputDirty = true;

        m_renderSettings->clearDirty();
    }

    if (m_render_result.width != m_renderSettings->getWidth() ||
        m_render_result.height != m_renderSettings->getHeight())
    {
        m_render_result.width = m_renderSettings->getWidth();
        m_render_result.height = m_renderSettings->getHeight();
        m_accumulation_buffer.resize(m_render_result.width * m_render_result.height * 4);
        m_render_result.image_buffer.resize(m_render_result.width * m_render_result.height);
        std::ranges::fill(m_accumulation_buffer, 0.0f);
        m_frameCount = 0;
        m_outputDirty = true;

        create_tiles(m_render_result.width, m_render_result.height);
    }

    if (m_frameCount == 0)
    {
        std::ranges::fill(m_accumulation_buffer, 0.0f);
    }

    if (needs_rebuild)
    {
        rebuild_scene();
        m_scene->markChangesProcessed();
    }
}

// TODO: error handling
bool CPUPathTracer::initialize_embree()
{
    assert(!m_embreeDevice && "Embree device already initialized");
    m_embreeDevice = rtcNewDevice("verbose=1,threads=1");
    assert(m_embreeDevice && "Failed to create Embree device");

    assert(!m_embreeScene && "Embree scene already initialized");
    m_embreeScene = rtcNewScene(m_embreeDevice);
    assert(m_embreeScene && "Failed to create Embree scene");

    // embree_geometry_id = rtcAttachGeometry(scene, sphere_geometry);

    rtcCommitScene(m_embreeScene);

    return m_embreeDevice != nullptr && m_embreeScene != nullptr;
}

void CPUPathTracer::cleanup_embree()
{
    assert(m_embreeDevice && "Embree device not initialized");
    rtcReleaseDevice(m_embreeDevice);
    m_embreeDevice = nullptr;
    assert(m_embreeScene && "Embree scene not initialized");
    rtcReleaseScene(m_embreeScene);
    m_embreeScene = nullptr;
}

uint32_t CPUPathTracer::get_rng_state(uint32_t width, uint32_t /*height*/, uint32_t x, uint32_t y,
                                      uint32_t frame) const
{
    return x + y * width + frame * RNG_PRIME;
}

bool CPUPathTracer::intersect_scene(const glm::vec3& origin, const glm::vec3& direction,
                                    RTCRayHit& rayhit) const
{
    rayhit.ray.org_x = origin.x;
    rayhit.ray.org_y = origin.y;
    rayhit.ray.org_z = origin.z;
    rayhit.ray.dir_x = direction.x;
    rayhit.ray.dir_y = direction.y;
    rayhit.ray.dir_z = direction.z;
    rayhit.ray.tnear = RAY_TNEAR;
    rayhit.ray.tfar = INFINITY;
    rayhit.ray.mask = 0xFFFFFFFF;
    rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(m_embreeScene, &rayhit);

    return rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID;
}

glm::vec3 CPUPathTracer::get_albedo(const RTCRayHit& rayhit) const
{
    glm::vec3 albedo = glm::vec3(0.7f);

    RTCGeometry geom = rtcGetGeometry(m_embreeScene, rayhit.hit.geomID);
    if (geom)
    {
        const SceneNode* node = (const SceneNode*)rtcGetGeometryUserData(geom);
        if (node)
        {
            auto material = m_scene->getMaterial(node->GetID());
            if (material)
            {
                albedo = material->get<glm::vec3>("albedo", glm::vec3(0.7f));
            }
        }
    }

    return albedo;
}

// Simple diffuse path tracing helpers
// ===================================

glm::vec4 CPUPathTracer::trace_ray(const glm::vec3& ray_origin, const glm::vec3& ray_direction,
                                   uint32_t& rng_state) const
{
    // Professional path tracing workflow with explicit BSDF/PDF evaluation
    // ====================================================================
    //
    // Monte Carlo rendering equation:
    // Lo(x, wo) = Le(x, wo) + ∫ f(x, wi, wo) * Li(x, wi) * cos(θ) dω
    //
    // Monte Carlo estimator:
    // Lo ≈ Le + (f * Li * cos(θ)) / pdf
    //
    // Where:
    // - f = BSDF (evaluate_bsdf)
    // - Li = incoming radiance (recursively traced)
    // - cos(θ) = angle between normal and wi
    // - pdf = probability of sampling wi (from sample_bsdf)

    glm::vec3 accumulated_color = glm::vec3(0.0f);
    glm::vec3 ray_throughput = glm::vec3(1.0f);

    glm::vec3 current_origin = ray_origin;
    glm::vec3 current_direction = ray_direction;

    // Path tracing loop
    int bounce_count = 0;
    while (bounce_count < MAX_BOUNCES)
    {
        // 1. Intersect scene
        RTCRayHit rayhit;
        if (!intersect_scene(current_origin, current_direction, rayhit)) [[unlikely]]
        {
            // Hit sky/environment
            accumulated_color += ray_throughput * sample_sky(current_direction);
            break;
        }

        // 2. Calculate hit position and normal
        const float hit_t = rayhit.ray.tfar;
        const glm::vec3 hit_pos = current_origin + current_direction * hit_t;

        const float nx = rayhit.hit.Ng_x;
        const float ny = rayhit.hit.Ng_y;
        const float nz = rayhit.hit.Ng_z;
        const float inv_len = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
        const glm::vec3 normal(nx * inv_len, ny * inv_len, nz * inv_len);

        // 3. Get material properties
        const glm::vec3 albedo = get_albedo(rayhit);
        // TODO: Get other material properties (metallic, roughness, etc.) when needed

        // ============================================================================
        // BSDF EVALUATION - Monte Carlo Estimator
        // ============================================================================
        // The rendering equation: Lo = ∫ BSDF(wi,wo) * Li(wi) * cos(θ) dω
        // Monte Carlo estimator:  Lo ≈ (BSDF * Li * cos(θ)) / PDF
        //
        // We need to:
        // 1. Sample a direction wi (material-dependent)
        // 2. Evaluate BSDF for that direction
        // 3. Get the PDF (probability) of sampling that direction
        // 4. Compute: (BSDF * cos(θ)) / PDF
        // ============================================================================

        glm::vec3 wo = -current_direction;  // Outgoing direction (toward camera)

        // STEP 1: Sample direction based on material type
        float pdf;
        glm::vec3 wi = sample_bsdf(albedo, normal, wo, rng_state, pdf);

        // STEP 2: Evaluate BSDF
        glm::vec3 bsdf = evaluate_bsdf(albedo, normal, wi, wo);

        // STEP 3: Compute cos(θ) - angle between sampled direction and normal
        float cos_theta = glm::max(glm::dot(wi, normal), 0.0f);

        // STEP 4: Monte Carlo estimator: (BSDF * cos(θ)) / PDF
        glm::vec3 contribution = (bsdf * cos_theta) / pdf;

        // STEP 5: Update ray throughput
        ray_throughput *= contribution;

        // 8. Russian roulette path termination
        bounce_count++;
        if (bounce_count > RUSSIAN_ROULETTE_START_BOUNCE)
        {
            const float max_throughput =
                glm::max(glm::max(ray_throughput.r, ray_throughput.g), ray_throughput.b);
            const float continuation_probability = glm::min(max_throughput, 0.95f);
            if (random_float(rng_state) > continuation_probability)
                break;
            ray_throughput /= continuation_probability;
        }

        // 9. Setup next ray
        current_origin = hit_pos + normal * RAY_EPSILON;
        current_direction = wi;
    }

    return glm::vec4(accumulated_color, 1.0f);
}

glm::vec3 CPUPathTracer::sample_bsdf(const glm::vec3& /*albedo*/, const glm::vec3& normal,
                                     const glm::vec3& /*wo*/, uint32_t& rng_state, float& pdf_out) const
{
    // Sample a direction based on material type and return the PDF
    // TODO: Dispatch based on material properties (metallic, roughness, etc.)
    // - Diffuse: cosine-weighted hemisphere
    // - Metal: sample around reflection direction based on roughness
    // - Glass: choose reflection or refraction based on Fresnel

    // For now: only diffuse materials
    return sample_hemisphere_cosine(normal, rng_state, pdf_out);
}

glm::vec3 CPUPathTracer::evaluate_bsdf(const glm::vec3& albedo, const glm::vec3& /*normal*/,
                                       const glm::vec3& /*wi*/, const glm::vec3& /*wo*/) const
{
    // Returns the actual BSDF value (how light scatters at this surface)
    // TODO: Dispatch based on material properties (metallic, roughness, etc.)
    //
    // For diffuse (Lambertian) material:
    //   BSDF = albedo / π
    //
    // For other materials:
    //   - Metal: Cook-Torrance microfacet BRDF
    //   - Glass: Fresnel dielectric BSDF (reflection + refraction)

    // For now: only diffuse (Lambertian)
    return albedo / glm::pi<float>();
}

glm::vec3 CPUPathTracer::sample_hemisphere_cosine(const glm::vec3& normal, uint32_t& rng_state,
                                                  float& pdf_out) const
{
    float u1 = random_float(rng_state);
    float u2 = random_float(rng_state);

    const float phi = 2.0f * glm::pi<float>() * u1;
    const float cos_theta = sqrtf(u2);
    const float sin_theta = sqrtf(1.0f - u2);

    // Build tangent space
    glm::vec3 up = (abs(normal.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
    glm::vec3 tangent = normalize(cross(up, normal));
    glm::vec3 bitangent = cross(normal, tangent);

    // Sample direction
    glm::vec3 sample_tangent(sin_theta * cosf(phi), sin_theta * sinf(phi), cos_theta);
    glm::vec3 wi = sample_tangent.x * tangent + sample_tangent.y * bitangent + sample_tangent.z * normal;

    // PDF for cosine-weighted hemisphere sampling: cos(θ) / π
    pdf_out = cos_theta / glm::pi<float>();

    return wi;
}

glm::vec3 CPUPathTracer::sample_sky(const glm::vec3& direction) const
{
    float t = 0.5f * (direction.y + 1.0f);                  // Map y from [-1,1] to [0,1]
    glm::vec3 sky_color = glm::vec3(0.5f, 0.7f, 1.0f);      // Light blue
    glm::vec3 horizon_color = glm::vec3(1.0f, 1.0f, 1.0f);  // White
    return glm::mix(horizon_color, sky_color, t);
}

float CPUPathTracer::random_float(uint32_t& state) const
{
    uint32_t result;
    state = state * 747796405 + 2891336453;
    result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    result = (result >> 22) ^ result;
    return ((float)result / 4294967295.0f);
}

void CPUPathTracer::rebuild_scene()
{
    assert(m_scene && "Scene not set before rebuilding Embree scene");
    assert(m_embreeScene && "Embree scene not initialized");

    render::Log::info("Rebuilding Embree scene from application scene...");

    // Clear existing Embree scene by recreating it
    rtcReleaseScene(m_embreeScene);
    m_embreeScene = rtcNewScene(m_embreeDevice);
    rtcSetSceneFlags(m_embreeScene, RTC_SCENE_FLAG_ROBUST);

    auto all_nodes = m_scene->GetAllNodes();
    for (const auto& [id, node] : all_nodes)
    {
        render::Log::debug("Processing node ID: {}, Name: {}, Type: {}", id, node->GetName(),
                           static_cast<int>(node->GetType()));

        switch (node->GetType())
        {
            case render::NodeType::SPHERE_OBJECT:
            {
                const auto* sphere = static_cast<const render::SphereObject*>(node);
                // Create Embree geometry for sphere
                RTCGeometry sphere_geometry =
                    rtcNewGeometry(m_embreeDevice, RTC_GEOMETRY_TYPE_SPHERE_POINT);

                // Set sphere vertex data (center + radius)
                struct Vertex
                {
                    float x, y, z, radius;
                };
                Vertex* vertices =
                    (Vertex*)rtcSetNewGeometryBuffer(sphere_geometry, RTC_BUFFER_TYPE_VERTEX, 0,
                                                     RTC_FORMAT_FLOAT4, sizeof(Vertex), 1);

                glm::vec3 pos = sphere->GetPosition();
                vertices[0].x = pos.x;
                vertices[0].y = pos.y;
                vertices[0].z = pos.z;
                vertices[0].radius = sphere->GetRadius();

                render::Log::debug("Created sphere '{}' at ({}, {}, {}) with radius {}",
                                   node->GetName(), pos.x, pos.y, pos.z, sphere->GetRadius());

                rtcSetGeometryUserData(sphere_geometry, (void*)sphere);
                rtcCommitGeometry(sphere_geometry);
                rtcAttachGeometry(m_embreeScene, sphere_geometry);
                rtcReleaseGeometry(sphere_geometry);
                break;
            }
            default:
            {
                render::Log::warn("Unknown node type: {}", static_cast<int>(node->GetType()));
                break;
            }
        }
    }

    rtcCommitScene(m_embreeScene);
}

void CPUPathTracer::create_tiles(uint32_t width, uint32_t height)
{
    m_tiles.clear();
    for (uint32_t y = 0; y < height; y += TILE_SIZE)
    {
        for (uint32_t x = 0; x < width; x += TILE_SIZE)
        {
            RenderTile tile;
            tile.x_start = x;
            tile.y_start = y;
            tile.x_end = glm::min(x + TILE_SIZE, width);
            tile.y_end = glm::min(y + TILE_SIZE, height);
            m_tiles.push_back(tile);
        }
    }
}
}  // namespace render