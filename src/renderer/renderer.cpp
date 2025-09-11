#include "renderer.h"
#include "RenderTarget.h"
#include "scene/Scene.h"

void Renderer::render(const Scene &scene, RenderTarget &target, uint32_t frame)
{
	// Pure interface - just delegate to the render target
	// Each render target implements its own rendering strategy
	target.render(scene, frame);
}