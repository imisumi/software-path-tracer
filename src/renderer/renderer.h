#pragma once

class Texture2D;
#include <memory>
#include <vector>
#include <cstdint>

#include <glm/glm.hpp>

class Renderer
{
public:
	Renderer() = default;

	void on_resize(uint32_t width, uint32_t height);
	void render(std::shared_ptr<class Scene> scene);

	uint32_t per_pixel(std::shared_ptr<class Scene> scene, glm::vec2 uv) const;
	const std::unique_ptr<Texture2D> &get_texture() const { return m_texture; }

private:
	std::unique_ptr<Texture2D> m_texture;
	std::vector<uint32_t> m_data;
};