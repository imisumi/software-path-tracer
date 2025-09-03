#include "GraphicsContext.h"

GraphicsContext *GraphicsContext::s_Instance = nullptr;
SDL_Renderer *GraphicsContext::s_SDLRenderer = nullptr;

#include <SDL3/SDL.h>
#include <cassert>

void GraphicsContext::init(SDL_Window *window)
{
	assert(s_Instance == nullptr && "GraphicsContext already initialized");
	s_Instance = new GraphicsContext();

	s_SDLRenderer = SDL_CreateRenderer(window, nullptr);
	assert(s_SDLRenderer && "Failed to create SDL_Renderer from window");
}

SDL_Renderer *GraphicsContext::getSDLRenderer()
{
	assert(s_SDLRenderer && "GraphicsContext not initialized");
	assert(s_Instance && "GraphicsContext instance not created");
	return s_SDLRenderer;
}
