#pragma once

#include <imgui.h>
#include <glm/vec2.hpp>

#include <cstdint>
#include <vector>
#include <memory>

#include "renderer/RenderTargetFactory.h"

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

	std::vector<uint32_t> m_viewport_data;

	std::unique_ptr<RenderTarget> m_render_target;
	std::shared_ptr<class Scene> m_scene;
	RenderTargetFactory::Type m_render_target_type = RenderTargetFactory::Type::CPU;
};