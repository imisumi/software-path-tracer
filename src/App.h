#pragma once

#include <imgui.h>
#include <glm/vec2.hpp>

#include <cstdint>
#include <vector>
#include <memory>

#include "renderer/RenderTargetFactory.h"

#include "render/Types.h"

#include "render/PathTracer.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class App
{
public:
	App();
	~App();

	void run();

private:
	static App *s_Instance;

	SDL_Window *m_window = nullptr;
	// SDL_Renderer *m_renderer = nullptr;

	ImVec4 m_clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	uint32_t m_width = 2560;
	uint32_t m_height = 1440;
	glm::vec2 m_viewport_dimensions = glm::vec2(256.0f, 256.0f);

	enum class ViewportMode
	{
		WINDOW_DIMENSIONS = 0,
		CUSTOM_SIZE_256,
		CUSTOM_SIZE_512,
		CUSTOM_SIZE_1024
	};

	ViewportMode m_viewport_mode = ViewportMode::CUSTOM_SIZE_512;
	std::vector<uint32_t> m_viewport_data;

	std::unique_ptr<RenderTarget> m_render_target;
	std::shared_ptr<class Scene> m_scene;

	std::unique_ptr<Texture2D> test_tex;

private:
	// std::unique_ptr<render::RenderEngine> m_render_engine;
	// std::shared_ptr<render::RenderSettings> m_render_settings;

	std::unique_ptr<render::PathTracer> m_path_tracer;
	// std::shared_ptr<render::RenderSettings> m_render_settings;
};