#pragma once

#include "render/RenderEngine.h"
#include <vector>
#include <memory>

// Forward declarations for Embree types (avoid including heavy headers in public interface)

// Forward declarations
class Scene;

namespace render
{

	/// CPU-based path tracing implementation using Embree for acceleration
	/// Clean, modern API for progressive path tracing
	class CPUPathTracer : public RenderEngine
	{
	public:
		CPUPathTracer();
		~CPUPathTracer();

		// RenderEngine interface implementation
		void startProgressive(std::shared_ptr<const Scene> scene, std::shared_ptr<RenderSettings> settings) override;
		void stopProgressive() override;
		bool isProgressiveReady() const override;
		const std::vector<uint32_t> &getProgressiveResult() override;

		// Dimensions and status
		uint32_t getWidth() const override { return m_renderSettings ? m_renderSettings->getWidth() : 0; }
		uint32_t getHeight() const override { return m_renderSettings ? m_renderSettings->getHeight() : 0; }
		uint32_t getCurrentSampleCount() const override { return m_frameCount; }
		bool isProgressiveRunning() const override { return m_progressiveRunning; }

		// Backend identification
		std::string getBackendName() const override { return "CPU Path Tracer (Embree)"; }
		BackendType getBackendType() const override { return BackendType::CPU_EMBREE; }
		RenderingTechnique getRenderingTechnique() const override { return RenderingTechnique::PATH_TRACING; }

		void render() override;

	private:
		typedef struct RTCDeviceTy *RTCDevice;
		typedef struct RTCSceneTy *RTCScene;
		// Embree device and scene management
		RTCDevice m_embreeDevice = nullptr;
		RTCScene m_embreeScene = nullptr;

		// Progressive state
		std::shared_ptr<const Scene> m_scene;
		std::shared_ptr<RenderSettings> m_renderSettings;
		uint32_t m_frameCount = 0;
		bool m_progressiveRunning = false;

		// Rendering buffers
		std::vector<float> m_accumulationBuffer; // RGBARGBA... high precision
		std::vector<uint32_t> m_outputBuffer;	 // RGBA8 packed for display
		bool m_outputDirty = true;

		// Dirty detection
		std::weak_ptr<const Scene> m_lastScene;
		uint32_t m_lastSceneVersion = 0;

		// Internal rendering methods
		void initializeEmbree();
		void cleanupEmbree();
		void resetAccumulation();
		void resizeBuffers();
		void updateOutputBuffer();
		glm::vec4 traceRay(const Ray &ray, uint32_t &rngState) const;
	};

}