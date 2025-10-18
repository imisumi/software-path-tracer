#pragma once

#include <imgui.h>
#include <glm/vec2.hpp>

#include <cstdint>
#include <vector>
#include <memory>


#include "render/Types.h"

#include "render/PathTracer.h"
#include "render/Scene.h"

#include "renderer/Texture2D.h"
#include "ui/panels/MaterialsPanel.h"
#include "ui/panels/SceneHierarchyPanel.h"
#include "ui/panels/PropertiesPanel.h"

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
	glm::vec2 m_viewport_dimensions = glm::vec2(512.0f, 512.0f);

	enum class ViewportMode
	{
		WINDOW_DIMENSIONS = 0,
		CUSTOM_SIZE_256,
		CUSTOM_SIZE_512,
		CUSTOM_SIZE_1024
	};

	ViewportMode m_viewport_mode = ViewportMode::CUSTOM_SIZE_1024;
	std::vector<uint32_t> m_viewport_data;

	std::unique_ptr<Texture2D> test_tex;

private:
	void renderMaterialPanel();

private:

	std::unique_ptr<render::PathTracer> m_path_tracer;
	std::shared_ptr<render::Scene> m_render_scene;
	// std::shared_ptr<render::RenderSettings> m_render_settings;

	// UI Panels
	std::unique_ptr<ui::MaterialsPanel> m_materials_panel;
	std::unique_ptr<ui::SceneHierarchyPanel> m_scene_hierarchy_panel;
	std::unique_ptr<ui::PropertiesPanel> m_properties_panel;

	// Selection state (shared between panels)
	render::NodeID m_selected_node_id = 0;
	render::MaterialDescriptor::Handle m_selected_material;
};