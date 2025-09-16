#include "CPUPathTracer.h"
#include <embree4/rtcore.h>
#include <algorithm> // For std::clamp

namespace render
{

	CPUPathTracer::CPUPathTracer()
	{
		// Initialize Embree device and setup ray tracing acceleration structures
		// This will contain the logic currently in EmbreeRenderTarget constructor
	}

	CPUPathTracer::~CPUPathTracer()
	{
		// Cleanup Embree resources
		// This will contain the logic currently in EmbreeRenderTarget destructor
	}

	void CPUPathTracer::startProgressive(std::shared_ptr<const Scene> scene, std::shared_ptr<RenderSettings> settings)
	{
		// Store new scene and settings
		m_scene = scene;
		m_renderSettings = settings;

		// Check for changes that require resets
		bool sceneChanged = (m_lastScene.lock() != scene);
		bool settingsChanged = settings->isDirty();

		if (sceneChanged)
		{
			// Scene pointer changed - full rebuild needed
			// TODO: Rebuild Embree scene when we implement it
			resizeBuffers(); // May need resize for new scene
			resetAccumulation();
			m_lastScene = scene;
			// TODO: m_lastSceneVersion = scene ? scene->getVersion() : 0;
		}
		else if (settingsChanged)
		{
			// Settings changed - may need buffer resize
			resizeBuffers();
			resetAccumulation();
			settings->clearDirty();
		}

		m_progressiveRunning = true;
	}

	void CPUPathTracer::stopProgressive()
	{
		m_progressiveRunning = false;
	}

	bool CPUPathTracer::isProgressiveReady() const
	{
		// For now, always ready when progressive is running
		return m_progressiveRunning;
	}

	const std::vector<uint32_t> &CPUPathTracer::getProgressiveResult()
	{
		if (m_outputDirty)
		{
			updateOutputBuffer();
			m_outputDirty = false;
		}
		return m_outputBuffer;
	}

	void CPUPathTracer::render()
	{
		for (uint32_t y = 0; y < getHeight(); ++y)
		{
			for (uint32_t x = 0; x < getWidth(); ++x)
			{

				uint8_t r = (x * 255) / getWidth();
				uint8_t g = (y * 255) / getHeight();
				uint8_t b = 128;
				uint8_t a = 255;
				// Generate and trace rays for each pixel
				m_outputBuffer[y * getWidth() + x] = (r << 24) | (g << 16) | (b << 8) | (a << 0);
			}
		}
	}

	void CPUPathTracer::initializeEmbree()
	{
		// Embree device initialization and scene setup
		// Will contain logic from current Scene::init_embree()
	}

	void CPUPathTracer::cleanupEmbree()
	{
		// Cleanup Embree device and scene
		// Will contain cleanup logic from current code
	}

	void CPUPathTracer::resetAccumulation()
	{
		m_frameCount = 0;
		// Clear accumulation buffer to zero
		std::fill(m_accumulationBuffer.begin(), m_accumulationBuffer.end(), 0.0f);
		m_outputDirty = true; // Output needs update
	}

	void CPUPathTracer::resizeBuffers()
	{
		if (!m_renderSettings)
			return;

		uint32_t width = m_renderSettings->getWidth();
		uint32_t height = m_renderSettings->getHeight();
		uint32_t pixelCount = width * height;

		// Resize accumulation buffer (4 floats per pixel: RGBA)
		m_accumulationBuffer.resize(pixelCount * 4, 0.0f);

		// Resize output buffer (1 uint32_t per pixel: packed RGBA8)
		m_outputBuffer.resize(pixelCount, 0);

		m_outputDirty = true;
	}

	void CPUPathTracer::updateOutputBuffer()
	{
		return;
		// std::fill(m_outputBuffer.begin(), m_outputBuffer.end(), 0xffffffff); // Default to black
		// return ;
		if (!m_renderSettings || m_frameCount == 0)
		{
			// Fill with black if no frames rendered
			std::fill(m_outputBuffer.begin(), m_outputBuffer.end(), 0xFF000000); // Black with full alpha
			return;
		}

		uint32_t pixelCount = m_renderSettings->getWidth() * m_renderSettings->getHeight();
		float exposure = m_renderSettings->getExposure();
		float invFrameCount = 1.0f / static_cast<float>(m_frameCount);

		for (uint32_t i = 0; i < pixelCount; ++i)
		{
			// Get accumulated RGBA values
			float r = m_accumulationBuffer[i * 4 + 0] * invFrameCount;
			float g = m_accumulationBuffer[i * 4 + 1] * invFrameCount;
			float b = m_accumulationBuffer[i * 4 + 2] * invFrameCount;
			float a = m_accumulationBuffer[i * 4 + 3] * invFrameCount;

			// Apply exposure tone mapping
			r *= exposure;
			g *= exposure;
			b *= exposure;

			// Clamp and convert to 8-bit
			// uint32_t r8 = static_cast<uint32_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
			// uint32_t g8 = static_cast<uint32_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
			// uint32_t b8 = static_cast<uint32_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));
			// uint32_t a8 = static_cast<uint32_t>(std::clamp(a * 255.0f, 0.0f, 255.0f));

			uint32_t r8 = 255;
			uint32_t g8 = 255;
			uint32_t b8 = 255;
			uint32_t a8 = 255;

			// Pack into RGBA8 (assuming little-endian: ABGR in memory)
			m_outputBuffer[i] = (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
		}
	}

	glm::vec4 CPUPathTracer::traceRay(const Ray &ray, uint32_t &rngState) const
	{
		// Path tracing ray traversal and lighting calculation
		// Will contain the main path tracing algorithm from current code
		return glm::vec4(0.0f);
	}

}