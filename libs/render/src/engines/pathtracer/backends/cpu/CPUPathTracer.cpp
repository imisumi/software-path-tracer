#include "CPUPathTracer.h"
#include <embree4/rtcore.h>
#include <algorithm> // For std::clamp

#include <cassert>
#include <memory>

#include <ranges>

#include "render/Scene.h"
#include "render/Types.h"

#include "render/Color.h"

namespace render
{

	CPUPathTracer::CPUPathTracer()
	{
		// Initialize Embree device and setup ray tracing acceleration structures
		// This will contain the logic currently in EmbreeRenderTarget constructor

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
		assert(m_scene && "Scene not set before rendering");
		invalidate();

		// Perform path tracing here using Embree

		for (uint32_t y = 0; y < m_render_result.height; y++)
		{
			for (uint32_t x = 0; x < m_render_result.width; x++)
			{
				// Simple gradient based on pixel position and frame count
				float r = (float)x / (float)m_render_result.width;
				float g = (float)y / (float)m_render_result.height;
				float b = 0.5f;
				float a = 1.0f;

				m_accumulation_buffer[4 * (y * m_render_result.width + x) + 0] += r;
				m_accumulation_buffer[4 * (y * m_render_result.width + x) + 1] += g;
				m_accumulation_buffer[4 * (y * m_render_result.width + x) + 2] += b;
				m_accumulation_buffer[4 * (y * m_render_result.width + x) + 3] += a;
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
		// if (m_scene->hasChanges())
		// {
		// 	// TODO: update embree
		// 	//  bitmask for different changes, some require embree rebuild some dont
		// 	m_frameCount = 0;
		// 	m_outputDirty = true;
		// }
		if (m_renderSettings->isDirty())
		{
			m_frameCount = 0;
			m_outputDirty = true;
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
}