#include "EmbreeRenderTarget.h"
#include "scene/Scene.h"
#include <embree4/rtcore.h>
#include <algorithm>
#include <limits>
#include <cstdio>
#include <glm/gtc/constants.hpp>

#ifdef _OPENMP
#include <omp.h>
#endif

// Simple random number generator
static float random_float(uint32_t &state)
{
	uint32_t result;
	state = state * 747796405 + 2891336453;
	result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
	result = (result >> 22) ^ result;
	return ((float)result / 4294967295.0f);
}

// Cosine-weighted hemisphere sampling for diffuse bounces
static glm::vec3 getRandomBounce(const glm::vec3 &normal, uint32_t &rng_state)
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

static uint32_t get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame)
{
	return x + y * width + frame * 982451653U; // Large prime for better distribution
}

EmbreeRenderTarget::EmbreeRenderTarget(uint32_t width, uint32_t height)
{
	m_texture = std::make_unique<Texture2D>(width, height, Texture2D::Format::RGBA8);
	m_floatData.resize(width * height, glm::vec4(0.0f));
	m_displayData.resize(width * height, 0);
	m_frameCount = 0;
}

void EmbreeRenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec3 &color)
{
	// Bounds checking removed - caller responsible for valid coordinates
	uint32_t index = y * getWidth() + x;
	m_floatData[index] += glm::vec4(color, 1.0f);
}

void EmbreeRenderTarget::setPixel(uint32_t x, uint32_t y, const glm::vec4 &color)
{
	// Bounds checking removed - caller responsible for valid coordinates
	uint32_t index = y * getWidth() + x;
	m_floatData[index] += color;
}

void EmbreeRenderTarget::updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	(void)x;
	(void)y;
	(void)width;
	(void)height;
	// For CPU rendering, we update the entire texture at once
	// This could be optimized to update only the specified region
	commitPixels();
}

uint32_t EmbreeRenderTarget::getWidth() const
{
	return m_texture ? m_texture->get_width() : 0;
}

uint32_t EmbreeRenderTarget::getHeight() const
{
	return m_texture ? m_texture->get_height() : 0;
}

void EmbreeRenderTarget::clear(const glm::vec3 &color)
{
	glm::vec4 clearColor = glm::vec4(color, 1.0f);
	std::fill(m_floatData.begin(), m_floatData.end(), clearColor);
	m_frameCount = 0;
}

void EmbreeRenderTarget::resize(uint32_t width, uint32_t height)
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

void EmbreeRenderTarget::commitPixels()
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

void EmbreeRenderTarget::render(const Scene &scene, uint32_t frame)
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

	// Get direct pointer to float data for better performance
	std::vector<glm::vec4> &float_data = m_floatData;
	const float inv_height = 1.0f / height;
	const float inv_width = 1.0f / width;

#ifdef _OPENMP
	// omp_set_num_threads(8);
#endif

	// Process individual pixels with direct memory access
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 16)
#endif
	for (int y = 0; y < (int)height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			const uint32_t index = y * width + x;

			// Path trace this pixel
			uint32_t rng_state = get_rng_state(width, height, x, y, frame);
			glm::vec3 ray_origin(0.0f, 0.0f, 0.0f);

			// Fast ray direction calculation
			const float aspect_ratio = (float)width / (float)height;

			float u = x * inv_width;
			float v = 1.0f - y * inv_height;
			float uv_x = (u * 2.0f - 1.0f) * aspect_ratio;
			float uv_y = v * 2.0f - 1.0f;

			float len = sqrtf(uv_x * uv_x + uv_y * uv_y + 1.0f);
			glm::vec3 ray_direction(uv_x / len, uv_y / len, 1.0f / len);

			glm::vec4 color = trace_ray(scene, ray_origin, ray_direction, rng_state);

			// Optimized accumulation - read once, modify, write once
			glm::vec4& pixel = float_data[index];
			pixel.x += color.x;
			pixel.y += color.y; 
			pixel.z += color.z;
			pixel.w += color.w;
		}
	}

	// Commit all pixels to the texture
	commitPixels();
}

void EmbreeRenderTarget::raygen_shader_single(const Scene &scene, uint32_t x, uint32_t y, uint32_t frame) const
{
	(void)frame; // Unused parameter
	const uint32_t width = getWidth();
	const uint32_t height = getHeight();
	const float aspect_ratio = (float)width / (float)height;

	// Fast UV calculation (avoid divisions)
	const float inv_width = 1.0f / width;
	const float inv_height = 1.0f / height;

	float u = x * inv_width;
	float v = 1.0f - y * inv_height; // Flip Y
	float uv_x = (u * 2.0f - 1.0f) * aspect_ratio;
	float uv_y = v * 2.0f - 1.0f;

	// Fast normalization
	float len = sqrtf(uv_x * uv_x + uv_y * uv_y + 1.0f);
	float dir_x = uv_x / len;
	float dir_y = uv_y / len;
	float dir_z = 1.0f / len;

	// Single ray intersection
	RTCRayHit rayhit;
	rayhit.ray.org_x = 0.0f;
	rayhit.ray.org_y = 0.0f;
	rayhit.ray.org_z = 0.0f;
	rayhit.ray.dir_x = dir_x;
	rayhit.ray.dir_y = dir_y;
	rayhit.ray.dir_z = dir_z;
	rayhit.ray.tnear = 0.0001f;
	rayhit.ray.tfar = std::numeric_limits<float>::infinity();
	rayhit.ray.mask = 0xFFFFFFFF;
	rayhit.ray.flags = 0;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

	RTCIntersectArguments args;
	rtcInitIntersectArguments(&args);
	rtcIntersect1(scene.scene, &rayhit, &args);

	// Direct memory write
	auto *float_data = const_cast<std::vector<glm::vec4> *>(&m_floatData);
	uint32_t index = y * width + x;

	if (index < float_data->size())
	{
		// Use path tracing with bounces
		uint32_t rng_state = get_rng_state(width, height, x, y, frame);
		glm::vec3 ray_origin(0.0f, 0.0f, 0.0f);
		glm::vec3 ray_direction(dir_x, dir_y, dir_z);

		glm::vec4 color = trace_ray(scene, ray_origin, ray_direction, rng_state);

		(*float_data)[index] += color;
	}
}

void EmbreeRenderTarget::raygen_shader_packet(const Scene &scene, uint32_t start_x, uint32_t start_y, uint32_t frame, uint32_t packet_size) const
{
	// Only support packet size 1 - use single ray shader
	(void)packet_size; // Unused parameter
	raygen_shader_single(scene, start_x, start_y, frame);
}

glm::vec4 EmbreeRenderTarget::trace_ray(const Scene &scene, const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, uint32_t &rng_state) const
{
	const int max_bounces = 4;
	glm::vec3 accumulated_color = glm::vec3(0.0f);
	glm::vec3 ray_throughput = glm::vec3(1.0f);

	glm::vec3 current_origin = ray_origin;
	glm::vec3 current_direction = ray_direction;

	// Pre-check debug normals to avoid per-bounce overhead
	const bool debug_normals = scene.debug_normals;

	// Unrolled path tracing loop for better branch prediction
	int bounce_count = 0;
	while (bounce_count < max_bounces)
	{
		// Optimized Embree ray setup
		RTCRayHit rayhit;
		rayhit.ray.org_x = current_origin.x;
		rayhit.ray.org_y = current_origin.y;
		rayhit.ray.org_z = current_origin.z;
		rayhit.ray.dir_x = current_direction.x;
		rayhit.ray.dir_y = current_direction.y;
		rayhit.ray.dir_z = current_direction.z;
		rayhit.ray.tnear = 0.001f;
		rayhit.ray.tfar = INFINITY;
		rayhit.ray.mask = 0xFFFFFFFF;
		rayhit.ray.flags = 0;
		rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

		rtcIntersect1(scene.scene, &rayhit);

		// Check for miss - optimize for common case (hit)
		// [[unlikely]]
		if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) [[unlikely]]
		{
			accumulated_color += ray_throughput * sample_sky(current_direction, scene);
			break;
		}

		// Hit path - calculate surface properties
		const float hit_t = rayhit.ray.tfar;
		current_origin.x += hit_t * current_direction.x;
		current_origin.y += hit_t * current_direction.y;
		current_origin.z += hit_t * current_direction.z;

		// Fast normal normalization
		const float nx = rayhit.hit.Ng_x;
		const float ny = rayhit.hit.Ng_y;
		const float nz = rayhit.hit.Ng_z;
		const float inv_len = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
		const float norm_x = nx * inv_len;
		const float norm_y = ny * inv_len;
		const float norm_z = nz * inv_len;

		// Debug normals early exit
		// add [[unlikely]]
		if (debug_normals) [[unlikely]]
		{
			return glm::vec4((norm_x + 1.0f) * 0.5f, (norm_y + 1.0f) * 0.5f, (norm_z + 1.0f) * 0.5f, 1.0f);
		}

		// Update throughput
		ray_throughput *= 0.7f;

		// Russian roulette after 2 bounces
		bounce_count++;
		if (bounce_count > 2)
		{
			const float continuation_probability = std::max({ray_throughput.r, ray_throughput.g, ray_throughput.b});
			if (random_float(rng_state) > continuation_probability)
				break;
			ray_throughput /= continuation_probability;
		}

		// Generate new ray direction
		glm::vec3 normal(norm_x, norm_y, norm_z);
		current_direction = getRandomBounce(normal, rng_state);

		// Offset origin for next bounce
		const float EPSILON = 1e-4f;
		current_origin.x += norm_x * EPSILON;
		current_origin.y += norm_y * EPSILON;
		current_origin.z += norm_z * EPSILON;
	}

	return glm::vec4(accumulated_color, 1.0f);
}

glm::vec4 EmbreeRenderTarget::trace_ray_single_bounce(const Scene &scene, const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, uint32_t &rng_state) const
{
	const int max_bounces = 4;
	glm::vec3 accumulated_color = glm::vec3(0.0f);
	glm::vec3 ray_throughput = glm::vec3(1.0f);

	glm::vec3 current_origin = ray_origin;
	glm::vec3 current_direction = ray_direction;

	for (int bounce = 0; bounce < max_bounces; ++bounce)
	{
		// Use rtcIntersect1 for all bounces (including first)
		RTCRayHit rayhit;
		rayhit.ray.org_x = current_origin.x;
		rayhit.ray.org_y = current_origin.y;
		rayhit.ray.org_z = current_origin.z;
		rayhit.ray.dir_x = current_direction.x;
		rayhit.ray.dir_y = current_direction.y;
		rayhit.ray.dir_z = current_direction.z;
		rayhit.ray.tnear = 0.0f;
		rayhit.ray.tfar = INFINITY;
		rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

		rtcIntersect1(scene.scene, &rayhit);

		if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
		{
			// Miss - add sky contribution and break
			float t = 0.5f * (current_direction.y + 1.0f);
			glm::vec3 sky_color = glm::mix(glm::vec3(1.0f), glm::vec3(0.5f, 0.7f, 1.0f), t);
			accumulated_color += ray_throughput * sky_color;
			break;
		}

		// Hit - calculate surface properties
		glm::vec3 hit_pos = current_origin + rayhit.ray.tfar * current_direction;
		glm::vec3 normal(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
		normal = glm::normalize(normal);

		// Simple material: diffuse with albedo 0.7
		glm::vec3 albedo(0.7f, 0.7f, 0.7f);
		ray_throughput *= albedo;

		// Russian roulette termination after 2 bounces
		if (bounce > 1)
		{
			float continuation_probability = std::max({ray_throughput.r, ray_throughput.g, ray_throughput.b});
			if (random_float(rng_state) > continuation_probability)
				break;
			ray_throughput /= continuation_probability;
		}

		// Generate new ray direction using cosine-weighted hemisphere sampling
		current_direction = getRandomBounce(normal, rng_state);
		const float EPSILON = 1e-4f;
		current_origin = hit_pos + normal * EPSILON;
	}

	return glm::vec4(accumulated_color, 1.0f);
}

uint32_t EmbreeRenderTarget::colorToRGBA(const glm::vec3 &color) const
{
	const uint8_t r = (uint8_t)(std::max(0.0f, std::min(1.0f, color.r)) * 255);
	const uint8_t g = (uint8_t)(std::max(0.0f, std::min(1.0f, color.g)) * 255);
	const uint8_t b = (uint8_t)(std::max(0.0f, std::min(1.0f, color.b)) * 255);
	const uint8_t a = 255;

	return (r << 24) | (g << 16) | (b << 8) | (a << 0);
}

glm::vec3 EmbreeRenderTarget::sample_sky(const glm::vec3 &direction, const Scene &scene) const
{
	float t = 0.5f * (direction.y + 1.0f);
	glm::vec3 sky_color = glm::mix(glm::vec3(1.0f), glm::vec3(0.5f, 0.7f, 1.0f), t);
	return sky_color;
}