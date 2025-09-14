#pragma once

#include "../RenderTarget.h"
#include "../Texture2D.h"
#include "../HitInfo.h"
#include <memory>
#include <vector>
#include <cstdint>

#include "../Ray.h"

class SimdRenderTarget : public RenderTarget
{
public:
	SimdRenderTarget(uint32_t width, uint32_t height);
	~SimdRenderTarget() = default;

	// Main rendering - implements CPU path tracing
	void render(const Scene &scene, uint32_t frame) override;

	// Utility methods
	void setPixel(uint32_t x, uint32_t y, const glm::vec3 &color) override;
	void setPixel(uint32_t x, uint32_t y, const glm::vec4 &color);
	void updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
	uint32_t getWidth() const override;
	uint32_t getHeight() const override;
	void clear(const glm::vec3 &color = glm::vec3(0.0f)) override;

	void resize(uint32_t width, uint32_t height);

	// For ImGui display - exposes the texture
	const std::unique_ptr<Texture2D> &getTexture() const { return m_texture; }

	// Direct access to pixel data for batch operations
	std::vector<glm::vec4> &getFloatData() { return m_floatData; }
	const std::vector<glm::vec4> &getFloatData() const { return m_floatData; }

	// Commit all pixel changes to the texture (call after rendering)
	void commitPixels();

private:
	uint32_t colorToRGBA(const glm::vec3 &color) const;

	std::unique_ptr<Texture2D> m_texture;
	std::vector<glm::vec4> m_floatData;
	std::vector<uint32_t> m_displayData;
	uint32_t m_frameCount = 0;
};