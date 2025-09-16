#pragma once

#include "Types.h"
#include <memory>
#include <string>

namespace render
{

	// Forward declaration
	class Scene;

	/// Base interface for all rendering engines
	/// Clean, modern API for different rendering techniques (path tracing, rasterization, etc.)


	//TODO: make this a factory only and have specific interfaces for path tracing, rasterization, etc.
	class RenderEngine
	{
	public:
		virtual ~RenderEngine() = default;

		// Progressive rendering interface (primary API)
		virtual void startProgressive(std::shared_ptr<const Scene> scene,
									  std::shared_ptr<RenderSettings> settings) = 0;
		virtual void stopProgressive() = 0;
		virtual bool isProgressiveReady() const = 0;
		virtual const std::vector<uint32_t> &getProgressiveResult() = 0;

		// Dimensions and status
		virtual uint32_t getWidth() const = 0;
		virtual uint32_t getHeight() const = 0;
		virtual uint32_t getCurrentSampleCount() const = 0;
		virtual bool isProgressiveRunning() const = 0;

		// Backend identification
		virtual std::string getBackendName() const = 0;
		virtual BackendType getBackendType() const = 0;
		virtual RenderingTechnique getRenderingTechnique() const = 0;

		// Optional: GPU interop for zero-copy display (future optimization)
		virtual bool supportsDirectGPUAccess() const { return false; }
		virtual void *getGPUTexture() { return nullptr; }

		static std::unique_ptr<RenderEngine> createRenderEngine(BackendType backend);

		// TEMP
		virtual void render() = 0;
	};

}