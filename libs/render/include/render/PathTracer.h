#pragma once

#include <memory>
#include <string>
#include <vector>

namespace render
{

	class Scene;
	class RenderSettings;

	class PathTracer
	{
	public:
		enum class BackendType
		{
			CPU_EMBREE,
			GPU_OPTIX,
			GPU_METAL
		};

		struct RenderResult
		{
			std::vector<uint32_t> image_buffer;
			uint32_t width = 0;
			uint32_t height = 0;
		};

	public:
		PathTracer() = default;
		virtual ~PathTracer() = default;

		// Pure virtual method to be implemented by derived classes
		virtual void render() = 0;

		virtual void set_scene(std::shared_ptr<Scene> scene) = 0;
		virtual void set_settings(std::shared_ptr<RenderSettings> settings) = 0;

		virtual std::shared_ptr<Scene> get_scene() const = 0;
		virtual std::shared_ptr<RenderSettings> get_settings() const = 0;

		// Backend identification
		virtual BackendType get_backend_type() const = 0;
		// TODO: stringview
		virtual std::string get_backend_name() const = 0;

		virtual const RenderResult &get_render_result() = 0;

		static std::unique_ptr<PathTracer> create_path_tracer(BackendType backend);
	};

	
} // namespace render
