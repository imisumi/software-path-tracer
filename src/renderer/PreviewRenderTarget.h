// #pragma once
//
// #include "RenderTarget.h"
// #include <vector>
//
// struct SDL_Texture;
// struct SDL_Renderer;
//
// class PreviewRenderTarget : public RenderTarget
// {
// public:
// 	PreviewRenderTarget(SDL_Renderer *sdl_renderer, uint32_t width, uint32_t height);
// 	~PreviewRenderTarget();
//
// 	void setPixel(uint32_t x, uint32_t y, const glm::vec3 &color) override;
// 	void updateRegion(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
// 	uint32_t getWidth() const override { return m_width; }
// 	uint32_t getHeight() const override { return m_height; }
// 	void clear(const glm::vec3 &color = glm::vec3(0.0f)) override;
//
// 	SDL_Texture *getSDLTexture() const { return m_sdl_texture; }
//
// private:
// 	uint32_t colorToRGBA(const glm::vec3 &color);
//
// 	SDL_Renderer *m_sdl_renderer;	// Reference (not owned)
// 	SDL_Texture *m_sdl_texture;		// Owned by us
// 	std::vector<uint32_t> m_buffer; // Our own buffer
// 	uint32_t m_width, m_height;
// };