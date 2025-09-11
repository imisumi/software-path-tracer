#include "CPURenderTarget.h"
#include "scene/Scene.h"
#include "geometry/sphere_data.h"
#include <algorithm>
#include <glm/gtc/constants.hpp>

float random_float(uint32_t &state)
{
	uint32_t result;

	state = state * 747796405 + 2891336453;
	result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return ((float)result / 4294967295.0f);
}

// Cosine-weighted hemisphere sampling for physically accurate diffuse bounces
glm::vec3 getRandomBounce(const glm::vec3 &normal, uint32_t &rng_state)
{
	// Two random numbers for spherical coordinates
	float u1 = random_float(rng_state);
	float u2 = random_float(rng_state);

	// Cosine-weighted sampling
	float cosTheta = sqrt(u1);
	float sinTheta = sqrt(1.0f - u1);
	float phi = 2.0f * glm::pi<float>() * u2;

	// Local coordinates (normal is +Z axis)
	float x = sinTheta * cos(phi);
	float y = sinTheta * sin(phi);
	float z = cosTheta;

	// Build orthonormal basis from normal
	glm::vec3 up = (abs(normal.z) < 0.999f) ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
	glm::vec3 tangent = normalize(cross(up, normal));
	glm::vec3 bitangent = cross(normal, tangent);

	// Transform to world space
	return x * tangent + y * bitangent + z * normal;
}

CPURenderTarget::CPURenderTarget(uint32_t width, uint32_t height)
{
	m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA8);
	m_floatData.resize(width * height, glm::vec4(0.0f));
	m_displayData.resize(width * height, 0);
	m_frameCount = 0;
}

void CPURenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec3 &color)
{
	if (x >= getWidth() || y >= getHeight())
		return;

	uint32_t index = y * getWidth() + x;
	// Accumulate color instead of overwriting
	m_floatData[index] += glm::vec4(color, 1.0f);
}

void CPURenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec4 &color)
{
	if (x >= getWidth() || y >= getHeight())
		return;

	uint32_t index = y * getWidth() + x;
	// Accumulate color instead of overwriting
	m_floatData[index] += color;
}

void CPURenderTarget::updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	// For CPU rendering, we update the entire texture at once
	// This could be optimized to update only the specified region
	commitPixels();
}

uint32_t CPURenderTarget::getWidth() const
{
	return m_texture ? m_texture->get_width() : 0;
}

uint32_t CPURenderTarget::getHeight() const
{
	return m_texture ? m_texture->get_height() : 0;
}

void CPURenderTarget::clear(const glm::vec3 &color)
{
	glm::vec4 clearColor = glm::vec4(color, 1.0f);
	std::fill(m_floatData.begin(), m_floatData.end(), clearColor);
	m_frameCount = 0;
}

void CPURenderTarget::resize(uint32_t width, uint32_t height)
{
	if (m_texture && width == m_texture->get_width() && height == m_texture->get_height())
	{
		return;
	}

	if (m_texture)
	{
		m_texture->resize(width, height);
	}
	else
	{
		m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA8);
	}

	m_floatData.resize(width * height, glm::vec4(0.0f));
	m_displayData.resize(width * height, 0);
	m_frameCount = 0;
}

void CPURenderTarget::commitPixels()
{
	if (m_texture && m_frameCount > 0)
	{
		// Convert float buffer to uint8 RGBA for display
		// Divide by frame count for averaging
		for (size_t i = 0; i < m_floatData.size(); ++i)
		{
			glm::vec3 averagedColor = glm::vec3(m_floatData[i]) / static_cast<float>(m_frameCount);
			m_displayData[i] = colorToRGBA(averagedColor);
		}
		m_texture->set_data(m_displayData);
	}
}

// uint32_t get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame)
// {
// 	// Better hash function to avoid patterns
// 	uint32_t seed = x * 1664525 + y * 22695477 + frame * 1013904223;
// 	seed = seed * 747796405 + 2891336453;
// 	return seed;
// }

// uint32_t get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame)
// {
// 	// TODO: needs optimization
// 	glm::vec2 coord{(float)x / (float)(width), (float)y /
// 												   (float)(height)};
// 	glm::vec2 num_pixels{(float)width, (float)height};
// 	coord = coord * num_pixels;
// 	uint32_t pixel_index = coord.x + coord.y * num_pixels.x;
// 	return (pixel_index + frame * 719393);
// }

// uint32_t get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame)
// {
// 	// Direct integer math - no float precision issues
// 	return x + y * width + frame * (width * height + 1);
// }

uint32_t get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame)
{
	return x + y * width + frame * 982451653U; // Large prime for better distribution
}
void CPURenderTarget::render(const Scene &scene, uint32_t frame)
{
	const uint32_t width = getWidth();
	const uint32_t height = getHeight();

	// Clear on first frame (frame == 1) to reset accumulation
	if (frame == 1)
	{
		clear(glm::vec3(0.0f, 0.0f, 0.0f));
	}

	// Increment frame count for accumulation
	m_frameCount++;

	// CPU path tracing - pixel by pixel

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			// RayGen shader - only generates ray
			glm::vec4 color = raygen_shader(scene, x, y, frame);

			// uint32_t rng_state = get_rng_state(width, height, x, y, frame);

			// // TraceRay - orchestrated by dispatch loop
			// glm::vec4 color = trace_ray(scene, ray.origin, ray.direction, rng_state);

			// Write to render target
			setPixel(x, y, color);
		}
	}

	// Commit all pixels to the texture
	commitPixels();
}

glm::vec4 CPURenderTarget::raygen_shader(const Scene &scene, uint32_t x, uint32_t y, uint32_t frame) const
{
	// Calculate UV coordinates from pixel position
	const uint32_t width = getWidth();
	const uint32_t height = getHeight();
	float u = (float)x / width;
	float v = (float)y / height;
	v = 1.0f - v; // Flip Y: make Y=0 bottom, Y=1 top (standard 3D)
	glm::vec2 uv = glm::vec2(u, v) * 2.0f - 1.0f;

	// Apply aspect ratio correction
	float aspect_ratio = (float)width / (float)height;
	uv.x *= aspect_ratio;

	// Ray setup - this is what RayGen should do (ONLY ray generation)
	Ray ray;
	ray.origin = glm::vec3(0.0f, 0.0f, 0.0f);
	ray.direction = glm::vec3(uv.x, uv.y, 1.0f);   // Can be unnormalized
	ray.direction = glm::normalize(ray.direction); // Normalize direction

	uint32_t rng_state = get_rng_state(width, height, x, y, frame);

	return trace_ray(scene, ray.origin, ray.direction, rng_state);
}

glm::vec4 CPURenderTarget::trace_ray(const Scene &scene, const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, uint32_t &rng_state) const
{
	const int max_bounces = 8;
	glm::vec3 accumulated_color = glm::vec3(0.0f);
	glm::vec3 ray_throughput = glm::vec3(1.0f);

	glm::vec3 current_origin = ray_origin;
	glm::vec3 current_direction = ray_direction;

	for (int bounce = 0; bounce < max_bounces; ++bounce)
	{
		// Find closest intersection
		HitInfo hit = intersect_scene(current_origin, current_direction, scene);

		// AnyHit shader (for transparency, alpha testing, etc.)
		if (hit.is_hit() && !anyhit_shader(current_origin, current_direction, hit))
		{
			// Hit was rejected by anyhit shader, continue tracing
			hit.clear();
		}

		if (!hit.is_hit())
		{
			// Miss shader - add sky contribution and break
			glm::vec4 miss_color = miss_shader(current_direction);
			accumulated_color += ray_throughput * glm::vec3(miss_color);
			break;
		}

		// Calculate surface properties
		HitInfo mutable_hit = hit;
		calculate_surface_properties(scene, current_origin, current_direction, mutable_hit);

		// Material has roughness = 1.0 (fully diffuse)
		// For diffuse materials, we don't add emission here, just bounce

		// Update throughput (for now, assume albedo of 0.7 for all materials)
		ray_throughput *= 0.7f;

		// Russian roulette termination
		if (bounce > 3)
		{
			float continuation_probability = std::max({ray_throughput.r, ray_throughput.g, ray_throughput.b});
			if (random_float(rng_state) > continuation_probability)
				break;
			ray_throughput /= continuation_probability;
		}

		// Generate new ray direction using cosine-weighted hemisphere sampling
		current_direction = getRandomBounce(mutable_hit.normal, rng_state);
		// current_origin = mutable_hit.position;
		const float EPSILON = 1e-5f;
		current_origin = mutable_hit.position + mutable_hit.normal * EPSILON;
	}

	return glm::vec4(accumulated_color, 1.0f);
}

HitInfo CPURenderTarget::intersect_scene(const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, const Scene &scene) const
{
	HitInfo closest_hit;
	closest_hit.clear();

	const SphereData &spheres = scene.get_sphere_data();
	if (spheres.size() == 0)
	{
		return closest_hit;
	}

	// Test all spheres, keep closest hit
	for (size_t i = 0; i < spheres.size(); ++i)
	{
		glm::vec3 sphere_center(spheres.cx[i], spheres.cy[i], spheres.cz[i]);
		float sphere_radius = spheres.radii[i];

		HitInfo current_hit;
		if (intersect_sphere(ray_origin, ray_direction, sphere_center, sphere_radius,
							 static_cast<uint32_t>(i), spheres.material_indices[i], current_hit))
		{
			// Keep closest hit
			if (!closest_hit.is_hit() || current_hit.t < closest_hit.t)
			{
				closest_hit = current_hit;
			}
		}
	}

	return closest_hit;
}

bool CPURenderTarget::intersect_sphere(const glm::vec3 &ray_origin, const glm::vec3 &ray_direction,
									   const glm::vec3 &sphere_center, float sphere_radius,
									   uint32_t sphere_id, uint32_t material_id, HitInfo &hit) const
{
	// Ray-sphere intersection using algebraic method
	glm::vec3 L = ray_origin - sphere_center;
	// float a = glm::dot(ray_direction, ray_direction);
	float a = 1.0f; // Assume ray_direction is normalized
	float b = 2.0f * glm::dot(L, ray_direction);
	float c = glm::dot(L, L) - sphere_radius * sphere_radius;

	float discriminant = b * b - 4 * a * c;
	if (discriminant < 0)
		return false; // No intersection

	float sqrt_discriminant = sqrt(discriminant);
	float t0 = (-b - sqrt_discriminant) / (2 * a);
	float t1 = (-b + sqrt_discriminant) / (2 * a);

	// Choose nearest positive intersection
	float t = (t0 > 0) ? t0 : (t1 > 0 ? t1 : -1.0f);
	if (t <= 0)
		return false;

	// Fill hit info - only essential data
	hit.t = t;
	hit.normal = glm::normalize(ray_origin + t * ray_direction - sphere_center);
	hit.position = ray_origin + t * ray_direction;
	// avoid self-intersection by offsetting position slightly along normal
	// hit.position += hit.normal * 0.001f;
	hit.object_id = sphere_id;
	hit.material_id = material_id;

	// Surface properties calculated on-demand in calculate_surface_properties()
	return true;
}

void CPURenderTarget::calculate_surface_properties(const Scene &scene, const glm::vec3 &ray_origin,
												   const glm::vec3 &ray_direction, HitInfo &hit) const
{
	if (!hit.is_hit())
		return;

	const SphereData &spheres = scene.get_sphere_data();
	if (hit.object_id >= spheres.size())
		return;

	// Calculate hit point
	hit.position = ray_origin + hit.t * ray_direction;

	// Calculate normal for sphere
	glm::vec3 sphere_center(spheres.cx[hit.object_id],
							spheres.cy[hit.object_id],
							spheres.cz[hit.object_id]);

	hit.normal = glm::normalize(hit.position - sphere_center);

	// UV coordinates (future - for texture mapping)
	hit.uv = glm::vec2(0.0f, 0.0f);
}

bool CPURenderTarget::anyhit_shader(const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, HitInfo &hit) const
{
	// Default: accept all hits
	// In the future, this can handle transparency, alpha testing, etc.
	return true;
}

glm::vec4 CPURenderTarget::closesthit_shader(const Scene &scene, const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, const HitInfo &hit) const
{
	// Calculate surface properties on-demand
	HitInfo mutable_hit = hit;
	calculate_surface_properties(scene, ray_origin, ray_direction, mutable_hit);

	// Convert normal to color (classic normal visualization)
	glm::vec3 color = (mutable_hit.normal + 1.0f) * 0.5f; // Map [-1,1] to [0,1]
	return glm::vec4(color, 1.0f);						  // Opaque
}

glm::vec4 CPURenderTarget::miss_shader(const glm::vec3 &ray_direction) const
{
	// Sky gradient (moved from sampleSky function)
	float t = 0.5f * (ray_direction.y + 1.0f);
	glm::vec3 color = glm::mix(glm::vec3(1.0f), glm::vec3(0.5f, 0.7f, 1.0f), t);
	return glm::vec4(color, 1.0f); // Opaque sky
}

uint32_t CPURenderTarget::colorToRGBA(const glm::vec3 &color) const
{
	const uint8_t r = (uint8_t)(std::clamp(color.r, 0.0f, 1.0f) * 255);
	const uint8_t g = (uint8_t)(std::clamp(color.g, 0.0f, 1.0f) * 255);
	const uint8_t b = (uint8_t)(std::clamp(color.b, 0.0f, 1.0f) * 255);
	const uint8_t a = 255;

	return (r << 24) | (g << 16) | (b << 8) | (a << 0);
}