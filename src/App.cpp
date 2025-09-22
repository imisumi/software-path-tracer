#include "App.h"
#include <assert.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <stdio.h>
#include <stdexcept>
#include <vector>
#include <iostream>

#include "renderer/GraphicsContext.h"

#include <SDL3/SDL.h>

#include <embree4/rtcore.h>

#include <glm/gtc/matrix_transform.hpp>

#include "render/Log.h"

// Factory handles the specific implementation

// Cross-platform SIMD includes
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h>
#include <pmmintrin.h>
#define HAS_X86_SIMD 1
#endif

static void SetDarkThemeColors();

App *App::s_Instance = nullptr;

App::App()
{
	assert(s_Instance == nullptr && "App already exists!");
	s_Instance = this;
	// Setup SDL
	// [If using SDL_MAIN_USE_CALLBACKS: all code below until the main loop starts would likely be your SDL_AppInit() function]
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
	{
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		throw std::runtime_error("Failed to initialize SDL");
	}

	// Create window with SDL_Renderer graphics context
	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	m_window = SDL_CreateWindow("Dear ImGui SDL3+SDL_Renderer example", (int)(m_width * main_scale), (int)(m_height * main_scale), window_flags);
	if (m_window == nullptr)
	{
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		throw std::runtime_error("Failed to create SDL m_window");
	}
	GraphicsContext::init(m_window);

	SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(m_window);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // Enable Docking

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	SetDarkThemeColors();
	// ImGui::StyleColorsLight();

	// Setup scaling
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
									 // io.ConfigDpiScaleFonts = true;        // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
									 // io.ConfigDpiScaleViewports = true;    // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	ImGui_ImplSDL3_InitForSDLRenderer(m_window, GraphicsContext::getSDLRenderer());
	ImGui_ImplSDLRenderer3_Init(GraphicsContext::getSDLRenderer());



	render::Log::set_level(render::LogLevel::Debug);
	render::Log::set_callback([](render::LogLevel level, std::string_view msg) {
		const char* level_str = "INFO";
		switch(level) {
			case render::LogLevel::Debug: level_str = "DEBUG"; break;
			case render::LogLevel::Warn:  level_str = "WARN";  break;
			case render::LogLevel::Error: level_str = "ERROR"; break;
		}
		std::cout << std::format("[RENDER] [{}] {}\n", level_str, msg);
	});

	{
		m_path_tracer = render::PathTracer::create_path_tracer(render::PathTracer::BackendType::CPU_EMBREE);
		m_render_scene = std::make_shared<render::Scene>();
		
		{
			auto sphere = m_render_scene->CreateNode<render::SphereObject>("123");
			sphere->SetRadius(1.0f);
			sphere->SetPosition(glm::vec3(0.0f, -1.0f, 5.0f));
		}

		{
			auto sphere = m_render_scene->CreateNode<render::SphereObject>("123");
			sphere->SetRadius(100.0f);
			sphere->SetPosition(glm::vec3(0.0f, -102.0f, 5.0f));
		}

		int dims = 5;
		for (int x = -dims; x <= dims; x += 2)
		{
			for (int y = -dims; y <= dims; y += 2)
			{
				auto s = m_render_scene->CreateNode<render::SphereObject>("sphere");
				s->SetRadius(0.5f);
				s->SetPosition(glm::vec3((float)x, (float)y, 10.0f));
			}
		}

		// Initialize render settings
		auto render_settings = std::make_shared<render::RenderSettings>();
		render_settings->setResolution(512, 512);
		render_settings->setSamplesPerPixel(64);
		render_settings->setMaxBounces(8);
		m_path_tracer->set_settings(render_settings);
		m_path_tracer->set_scene(m_render_scene);

		test_tex = std::make_unique<Texture2D>(512, 512, Texture2D::Format::RGBA8);
	}
}

App::~App()
{
	// Cleanup
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	// SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void App::run()
{

	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	// Main loop
	bool done = false;
	uint32_t frame = 1;
	while (!done)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		// [If using SDL_MAIN_USE_CALLBACKS: call ImGui_ImplSDL3_ProcessEvent() from your SDL_AppEvent() function]
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			ImGui_ImplSDL3_ProcessEvent(&event);
			if (event.type == SDL_EVENT_QUIT)
				done = true;
			if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_window))
				done = true;
		}

		// [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppIterate() function]
		if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MINIMIZED)
		{
			SDL_Delay(10);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());

		{
			ImGui::Begin("Viewport");

			ImVec2 content_region = ImGui::GetContentRegionAvail();
			glm::vec2 new_viewport_dimensions;

			// Calculate viewport dimensions based on mode
			if (m_viewport_mode == ViewportMode::WINDOW_DIMENSIONS)
			{
				new_viewport_dimensions = glm::vec2(content_region.x, content_region.y);
			}
			else
			{
				// Custom texture sizes
				int custom_size = 256;
				switch (m_viewport_mode)
				{
				case ViewportMode::CUSTOM_SIZE_256:
					custom_size = 256;
					break;
				case ViewportMode::CUSTOM_SIZE_512:
					custom_size = 512;
					break;
				case ViewportMode::CUSTOM_SIZE_1024:
					custom_size = 1024;
					break;
				default:
					custom_size = 256;
					break;
				}
				new_viewport_dimensions = glm::vec2(custom_size, custom_size);
			}

			if (new_viewport_dimensions.x != m_viewport_dimensions.x || new_viewport_dimensions.y != m_viewport_dimensions.y)
			{
				m_viewport_dimensions = new_viewport_dimensions;

				// Recreate render target with new size - Embree only
				// m_render_target = RenderTargetFactory::create((int)m_viewport_dimensions.x,
				// 											  (int)m_viewport_dimensions.y);
			}

			{
				m_path_tracer->render();
				const auto &result = m_path_tracer->get_render_result();
				if (result.width > 0 && result.height > 0)
				{
					
					SDL_UpdateTexture((SDL_Texture *)test_tex->get_texture(), nullptr,
									  result.image_buffer.data(),
									  result.width * sizeof(uint32_t));
				}
			}

			frame++;

			// Calculate display size based on viewport mode
			ImVec2 display_size;
			if (m_viewport_mode == ViewportMode::WINDOW_DIMENSIONS)
			{
				// Fill the entire window
				display_size = content_region;
			}
			else
			{
				// Fit custom texture size within window, maintaining aspect ratio
				float texture_aspect = m_viewport_dimensions.x / m_viewport_dimensions.y;
				float window_aspect = content_region.x / content_region.y;

				if (texture_aspect > window_aspect)
				{
					// Texture is wider, fit to window width
					display_size.x = content_region.x;
					display_size.y = content_region.x / texture_aspect;
				}
				else
				{
					// Texture is taller, fit to window height
					display_size.x = content_region.y * texture_aspect;
					display_size.y = content_region.y;
				}
			}

			{
				ImGui::Image((SDL_Texture *)test_tex->get_texture(), display_size);
			}

			ImGui::End();
		}

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Properties");

			ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);			   // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", (float *)&m_clear_color); // Edit 3 floats representing a color

			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);


			// Viewport mode selection
			ImGui::Separator();
			ImGui::Text("Viewport Mode:");
			const char *viewport_modes[] = {"Window Dimensions", "Custom 256x256", "Custom 512x512", "Custom 1024x1024"};
			int current_mode = static_cast<int>(m_viewport_mode);

			if (ImGui::Combo("Mode", &current_mode, viewport_modes, 4))
			{
				m_viewport_mode = static_cast<ViewportMode>(current_mode);
				frame = 1; // Reset frame counter to clear accumulated samples
			}

			// Renderer info
			ImGui::Separator();
			ImGui::Text("Renderer Backend: Embree");

			// Tonemapping controls
			ImGui::Separator();

			// Debug options
			ImGui::Separator();
			ImGui::Text("Debug Options:");

			ImGui::Separator();

			ImGui::End();
		}

		// Rendering
		ImGui::Render();
		SDL_SetRenderScale(GraphicsContext::getSDLRenderer(), io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColorFloat(GraphicsContext::getSDLRenderer(), m_clear_color.x, m_clear_color.y, m_clear_color.z, m_clear_color.w);
		SDL_RenderClear(GraphicsContext::getSDLRenderer());
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), GraphicsContext::getSDLRenderer());
		SDL_RenderPresent(GraphicsContext::getSDLRenderer());
	}
}

static void SetDarkThemeColors()
{
	ImGuiStyle &style = ImGui::GetStyle();
	ImVec4 *colors = style.Colors;

	// Keep the same spacing and rounding
	style.WindowRounding = 6.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowPadding = ImVec2(12, 12);
	style.FramePadding = ImVec2(6, 4);
	style.FrameRounding = 4.0f;
	style.ItemSpacing = ImVec2(8, 6);
	style.ItemInnerSpacing = ImVec2(6, 4);
	style.IndentSpacing = 22.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 8.0f;
	style.GrabMinSize = 12.0f;
	style.GrabRounding = 3.0f;
	style.PopupRounding = 4.0f;

	// Base colors
	colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);			// Light grey text (not pure white)
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f); // Medium grey for disabled
	colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.95f);		// Dark m_window background
	colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 0.95f);		// Match m_window background
	colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.95f);		// Slightly darker than m_window
	colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);		// Dark grey border
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);		// No shadow

	// Frame colors
	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 0.95f);		  // Dark element backgrounds
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.20f, 0.95f); // Slightly lighter on hover
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);  // Even lighter when active

	// Title bar colors
	colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);			// Dark grey inactive title
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);	// Slightly lighter active title
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f); // Transparent when collapsed
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);		// Slightly darker than title

	// Scrollbar colors
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.95f);			// Scrollbar background
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);		// Scrollbar grab
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Scrollbar grab when hovered
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);	// Scrollbar grab when active

	// Widget colors
	colors[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);		// Light grey checkmark
	colors[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);		// Slider grab
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f); // Slider grab when active
	colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 0.80f);			// Dark grey buttons
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);	// Slightly lighter on hover
	colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);		// Even lighter when active

	// Header colors (TreeNode, Selectable, etc)
	colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 0.76f);		 // Pure dark grey
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.80f); // Slightly lighter on hover
	colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);	 // Even lighter when active

	// Separator
	colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);		// Separator color
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f); // Separator when hovered
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);	// Separator when active

	// Resize grip
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);		 // Resize grip
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.30f, 0.30f, 0.30f, 0.75f); // Resize grip when hovered
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);	 // Resize grip when active

	// Text input cursor
	colors[ImGuiCol_InputTextCursor] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Text input cursor

	// ALL TAB COLORS (both old and new names)
	// Using the newer tab color naming from your enum
	colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.15f, 0.86f);						 // Unselected tab
	colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.80f);				 // Tab when hovered
	colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);				 // Selected tab
	colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);		 // Selected tab overline
	colors[ImGuiCol_TabDimmed] = ImVec4(0.13f, 0.13f, 0.13f, 0.86f);				 // Dimmed/unfocused tab
	colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);		 // Selected but unfocused tab
	colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Overline of unfocused selected tab

	colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);		  // Active tab (old name)
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.13f, 0.13f, 0.13f, 0.86f);		  // Unfocused tab (old name)
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f); // Unfocused active tab (old name)

	// Docking colors
	colors[ImGuiCol_DockingPreview] = ImVec4(0.30f, 0.30f, 0.30f, 0.40f); // Preview when docking
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f); // Empty docking space

	// Plot colors
	colors[ImGuiCol_PlotLines] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);			// Plot lines
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);		// Plot lines when hovered
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);		// Plot histogram
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); // Plot histogram when hovered

	// Table colors
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);	 // Table header background
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f); // Table outer borders
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);	 // Table inner borders
	colors[ImGuiCol_TableRowBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.90f);		 // Table row background (even)
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.16f, 0.16f, 0.16f, 0.90f);	 // Table row background (odd)

	// Miscellaneous
	colors[ImGuiCol_TextLink] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);				 // Light grey for links (not blue)
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.30f, 0.30f, 0.30f, 0.35f);		 // Light grey selection background
	colors[ImGuiCol_TreeLines] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);			 // Tree node lines
	colors[ImGuiCol_DragDropTarget] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);		 // Drag and drop target
	colors[ImGuiCol_NavCursor] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);			 // Navigation cursor
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.40f, 0.40f, 0.40f, 0.70f); // Nav windowing highlight
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);	 // Nav windowing dim
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.75f);		 // Modal m_window dim
}
