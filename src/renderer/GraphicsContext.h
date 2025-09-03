#pragma once

struct SDL_Renderer;
struct SDL_Window;

class GraphicsContext
{
public:
	static void init(SDL_Window *window);

	static SDL_Renderer *getSDLRenderer();

private:
	GraphicsContext() = default;

private:
	static GraphicsContext *s_Instance;
	static SDL_Renderer *s_SDLRenderer;
};