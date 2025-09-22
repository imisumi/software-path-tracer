#include "CPUPathTracer.h"
#include <embree4/rtcore.h>
#include <algorithm> // For std::clamp

#include <cassert>
#include <memory>

#include <ranges>

#include "render/Scene.h"
#include "render/Types.h"

#include "render/Color.h"

#include <glm/gtc/constants.hpp>
#include <iostream>

#include "render/Log.h"

#include "render_assert.h"

namespace render
{

	CPUPathTracer::CPUPathTracer()
	{
		// Initialize Embree device and setup ray tracing acceleration structures
		// This will contain the logic currently in EmbreeRenderTarget constructor

		std::cout << "Initializing CPU Path Tracer with Embree backend..." << std::endl;
		render::Log::info("Initializing CPU Path Tracer with Embree backend...");

		m_renderSettings = std::make_shared<RenderSettings>();
		initialize_embree();
	}

	CPUPathTracer::~CPUPathTracer()
	{
		// Cleanup Embree resources
		// This will contain the logic currently in EmbreeRenderTarget destructor
	}

	void CPUPathTracer::render()
	{
		verify(m_embreeDevice && m_embreeScene, "Embree not initialized");
		verify(m_scene != nullptr, "Scene not set before rendering");
		invalidate();

		// Perform path tracing here using Embree
		const uint32_t width = m_render_result.width;
		const uint32_t height = m_render_result.height;

		const float inv_height = 1.0f / height;
		const float inv_width = 1.0f / width;


		for (uint32_t y = 0; y < height; y++)
		{
			for (uint32_t x = 0; x < width; x++)
			{
				uint32_t rng_state = get_rng_state(width, height, x, y, m_frameCount + 1);
				glm::vec3 ray_origin(0.0f, 0.0f, 0.0f);

				// Fast ray direction calculation
				const float aspect_ratio = (float)width / (float)height;

				float u = x * inv_width;
				float v = 1.0f - y * inv_height;
				float uv_x = (u * 2.0f - 1.0f) * aspect_ratio;
				float uv_y = v * 2.0f - 1.0f;

				float len = sqrtf(uv_x * uv_x + uv_y * uv_y + 1.0f);
				glm::vec3 ray_direction(uv_x / len, uv_y / len, 1.0f / len);

				glm::vec4 color = trace_ray(ray_origin, ray_direction, rng_state);

				m_accumulation_buffer[4 * (y * width + x) + 0] += color.r;
				m_accumulation_buffer[4 * (y * width + x) + 1] += color.g;
				m_accumulation_buffer[4 * (y * width + x) + 2] += color.b;
				m_accumulation_buffer[4 * (y * width + x) + 3] += color.a;
			}
		}

		m_frameCount++;
	}

	const PathTracer::RenderResult &CPUPathTracer::get_render_result()
	{
		assert(m_frameCount > 0 && "No frames rendered yet");
		// Convert accumulation buffer to 8-bit RGBA for output
		for (uint32_t y = 0; y < m_render_result.height; y++)
		{
			for (uint32_t x = 0; x < m_render_result.width; x++)
			{
				uint32_t idx = y * m_render_result.width + x;
				float r = m_accumulation_buffer[4 * idx + 0] / (float)m_frameCount;
				float g = m_accumulation_buffer[4 * idx + 1] / (float)m_frameCount;
				float b = m_accumulation_buffer[4 * idx + 2] / (float)m_frameCount;
				float a = m_accumulation_buffer[4 * idx + 3] / (float)m_frameCount;

				// Apply exposure
				// r *= m_renderSettings->getExposure();
				// g *= m_renderSettings->getExposure();
				// b *= m_renderSettings->getExposure();

				// Clamp to [0,1]
				r = std::clamp(r, 0.0f, 1.0f);
				g = std::clamp(g, 0.0f, 1.0f);
				b = std::clamp(b, 0.0f, 1.0f);
				a = std::clamp(a, 0.0f, 1.0f);

				m_render_result.image_buffer[idx] = rgba_to_uint32((uint8_t)(r * 255.0f), (uint8_t)(g * 255.0f), (uint8_t)(b * 255.0f), (uint8_t)(a * 255.0f));
			}
		}

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

		if (m_render_result.width != m_renderSettings->getWidth() || m_render_result.height != m_renderSettings->getHeight())
		{
			m_render_result.width = m_renderSettings->getWidth();
			m_render_result.height = m_renderSettings->getHeight();
			m_accumulation_buffer.resize(m_render_result.width * m_render_result.height * 4);
			m_render_result.image_buffer.resize(m_render_result.width * m_render_result.height);
			std::ranges::fill(m_accumulation_buffer, 0.0f);
			m_frameCount = 0;
			m_outputDirty = true;
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
		m_embreeDevice = rtcNewDevice("verbose=1,threads=0");
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
	
	uint32_t CPUPathTracer::get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame) const
	{
			return x + y * width + frame * 982451653U; // Large prime for better distribution
	}
	
	glm::vec4 CPUPathTracer::trace_ray(const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, uint32_t &rng_state) const
	{
		const int max_bounces = 4;
		glm::vec3 accumulated_color = glm::vec3(0.0f);
		glm::vec3 ray_throughput = glm::vec3(1.0f);

		glm::vec3 current_origin = ray_origin;
		glm::vec3 current_direction = ray_direction;

		// Pre-check debug normals to avoid per-bounce overhead
		const bool debug_normals = false;

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

			rtcIntersect1(m_embreeScene, &rayhit);

			// Check for miss - optimize for common case (hit)
			// [[unlikely]]
			if (rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) [[unlikely]]
			{
				accumulated_color += ray_throughput * sample_sky(current_direction);
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
			current_direction = get_random_bounche(normal, rng_state);

			// Offset origin for next bounce
			const float EPSILON = 1e-4f;
			current_origin.x += norm_x * EPSILON;
			current_origin.y += norm_y * EPSILON;
			current_origin.z += norm_z * EPSILON;
		}

		return glm::vec4(accumulated_color, 1.0f);
	}
	
	glm::vec3 CPUPathTracer::sample_sky(const glm::vec3 &direction) const
	{
		float t = 0.5f * (direction.y + 1.0f); // Map y from [-1,1] to [0,1]
		glm::vec3 sky_color = glm::vec3(0.5f, 0.7f, 1.0f);    // Light blue
		glm::vec3 horizon_color = glm::vec3(1.0f, 1.0f, 1.0f); // White
		return glm::mix(horizon_color, sky_color, t);
	}
	
	float CPUPathTracer::random_float(uint32_t &state) const
	{
		uint32_t result;
		state = state * 747796405 + 2891336453;
		result = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
		result = (result >> 22) ^ result;
		return ((float)result / 4294967295.0f);
	}
	
	glm::vec3 CPUPathTracer::get_random_bounche(const glm::vec3 &normal, uint32_t &state) const
	{
		// Two random numbers for spherical coordinates
		float u1 = random_float(state);
		float u2 = random_float(state);

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
	
	void CPUPathTracer::rebuild_scene()
	{
		assert(m_scene && "Scene not set before rebuilding Embree scene");
		assert(m_embreeScene && "Embree scene not initialized");

		render::Log::info("Rebuilding Embree scene from application scene...");

		// const auto& objects = m_scene->getAllObjects();

		// for (const auto& obj : objects)
		// {
		// 	switch (obj->getType())
		// 	{
		// 		case Scene::NodeType::Sphere:
		// 		{
		// 			const auto* sphere = static_cast<const Scene::SphereObject*>(obj.get());

					// RTCGeometry sphere_geometry = rtcNewGeometry(m_embreeDevice, RTC_GEOMETRY_TYPE_SPHERE_POINT);
		// 			struct SpherePoint { float x, y, z, radius; };
		// 			SpherePoint* vertices = (SpherePoint*)rtcSetNewGeometryBuffer(sphere_geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(SpherePoint), 1);
		// 			vertices[0].x = sphere->transform[3][0];
		// 			vertices[0].y = sphere->transform[3][1];
		// 			vertices[0].z = sphere->transform[3][2];
		// 			vertices[0].radius = sphere->radius;

		// 			rtcCommitGeometry(sphere_geometry);
		// 			rtcAttachGeometry(m_embreeScene, sphere_geometry);
		// 			rtcReleaseGeometry(sphere_geometry);
		// 			break;
		// 		}
		// 	}
		// }


		auto all_nodes = m_scene->GetAllNodes();
		for (const auto& [id, node] : all_nodes)
		{
			std::cout << "Processing node ID: " << id << ", Name: " << node->GetName() << std::endl;
			std::cout << "Node Type: " << static_cast<int>(node->GetType()) << std::endl;
			switch (node->GetType())
			{
				case render::NodeType::SPHERE_OBJECT:
				{
					std::cout << "Creating sphere geometry for node: " << node->GetName() << std::endl;
					const auto* sphere = static_cast<const render::SphereObject*>(node);
					// Create Embree geometry for sphere
					RTCGeometry sphere_geometry = rtcNewGeometry(m_embreeDevice, RTC_GEOMETRY_TYPE_SPHERE_POINT);
					
					// Set sphere vertex data (center + radius)
					struct Vertex { float x, y, z, radius; };
					Vertex* vertices = (Vertex*)rtcSetNewGeometryBuffer(sphere_geometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(Vertex), 1);
					
					glm::vec3 pos = sphere->GetPosition();
					vertices[0].x = pos.x;
					vertices[0].y = pos.y;
					vertices[0].z = pos.z;
					vertices[0].radius = sphere->GetRadius();
					
					std::cout << "Sphere position: (" << pos.x << ", " << pos.y << ", " << pos.z << "), radius: " << sphere->GetRadius() << std::endl;
					
					rtcSetGeometryUserData(sphere_geometry, (void*)sphere);
					rtcCommitGeometry(sphere_geometry);
					rtcAttachGeometry(m_embreeScene, sphere_geometry);
					rtcReleaseGeometry(sphere_geometry);
					break;
				}
				default:
				{
					std::cout << "Unknown node type: " << static_cast<int>(node->GetType()) << std::endl;
					break;
				}
			}
		}


		rtcCommitScene(m_embreeScene);
	}
}