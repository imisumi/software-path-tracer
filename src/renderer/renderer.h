#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <memory>

class RenderTarget;
class Scene;

// Pure interface - just coordinates rendering
// No implementation details - delegates to RenderTarget
class Renderer
{
public:
	// Static interface - just delegates to render target
	static void render(const Scene &scene, RenderTarget &target, uint32_t frame);

private:
	// No member variables - pure static interface
};