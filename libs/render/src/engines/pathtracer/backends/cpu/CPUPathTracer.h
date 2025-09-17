#pragma once

#include "render/PathTracer.h"
#include <vector>
#include <memory>

// Forward declarations for Embree types (avoid including heavy headers in public interface)

// Forward declarations
class Scene;

typedef struct RTCDeviceTy *RTCDevice;
typedef struct RTCSceneTy *RTCScene;

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

	private:

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
	};

}