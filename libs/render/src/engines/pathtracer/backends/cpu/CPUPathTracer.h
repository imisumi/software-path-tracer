#pragma once

#include "render/PathTracer.h"
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <queue>

// Forward declarations for Embree types (avoid including heavy headers in public interface)

// Forward declarations
class Scene;

typedef struct RTCDeviceTy *RTCDevice;
typedef struct RTCSceneTy *RTCScene;
typedef struct RTCRayHit RTCRayHit;

namespace render
{
	/// CPU-based path tracing implementation using Embree for acceleration
	/// Clean, modern API for progressive path tracing
	class CPUPathTracer : public PathTracer
	{
	public:
		CPUPathTracer();
		~CPUPathTracer();

		void render() override;

		void set_scene(std::shared_ptr<Scene> scene) override { m_scene = scene; }
		void set_settings(std::shared_ptr<RenderSettings> settings) override { m_renderSettings = settings; }

		std::shared_ptr<Scene> get_scene() const override { return m_scene; }
		std::shared_ptr<RenderSettings> get_settings() const override { return m_renderSettings; }

		// Backend identification
		std::string get_backend_name() const override { return "CPU Path Tracer (Embree)"; }
		BackendType get_backend_type() const override { return BackendType::CPU_EMBREE; }

		const PathTracer::RenderResult &get_render_result() override;

	private:
		void invalidate();

		bool initialize_embree();
		void cleanup_embree();

		// Ray tracing pipeline
		uint32_t get_rng_state(uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t frame) const;
		glm::vec3 generate_camera_ray(uint32_t x, uint32_t y, uint32_t width, uint32_t height) const;
		glm::vec4 trace_ray(const glm::vec3 &ray_origin, const glm::vec3 &ray_direction, uint32_t &rng_state) const;
		bool intersect_scene(const glm::vec3 &origin, const glm::vec3 &direction, RTCRayHit &rayhit) const;

		// Material/Shading
		glm::vec3 get_albedo(const RTCRayHit &rayhit) const;

		// BSDF
		glm::vec3 sample_bsdf(const glm::vec3 &albedo, const glm::vec3 &normal,
		                      const glm::vec3 &wo, uint32_t &rng_state, float &pdf_out) const;
		glm::vec3 evaluate_bsdf(const glm::vec3 &albedo, const glm::vec3 &normal,
		                        const glm::vec3 &wi, const glm::vec3 &wo) const;

		// Sampling helpers
		glm::vec3 sample_hemisphere_cosine(const glm::vec3 &normal, uint32_t &rng_state, float &pdf_out) const;

		// Environment/Sky
		glm::vec3 sample_sky(const glm::vec3 &direction) const;

		// Utility
		float random_float(uint32_t &state) const;

		void rebuild_scene();

	private:
		// Rendering constants
		static constexpr int MAX_BOUNCES = 4;
		static constexpr int RUSSIAN_ROULETTE_START_BOUNCE = 2;
		static constexpr float RAY_EPSILON = 1e-4f;
		static constexpr float RAY_TNEAR = 0.001f;
		static constexpr uint32_t RNG_PRIME = 982451653U;

		// Embree device and scene management
		RTCDevice m_embreeDevice = nullptr;
		RTCScene m_embreeScene = nullptr;

		// Progressive state
		std::shared_ptr<Scene> m_scene;

		PathTracer::RenderResult m_render_result;

		uint32_t m_frameCount = 0;
		bool m_progressiveRunning = false;

		// Rendering buffers
		std::vector<float> m_accumulation_buffer; // RGBARGBA... high precision
		std::shared_ptr<RenderSettings> m_renderSettings;
		bool m_outputDirty = true;


		// threading
		struct RenderTile
		{
			uint32_t x_start;
			uint32_t y_start;
			uint32_t x_end;
			uint32_t y_end;
		};

		static constexpr int TILE_SIZE = 32;
		std::vector<RenderTile> m_tiles;

		void create_tiles(uint32_t width, uint32_t height);
	};

}