#pragma once

#include <glm/glm.hpp>

class Material
{
public:
	Material(const glm::vec3 &albedo) : m_albedo(albedo) {}

	const glm::vec3 &get_albedo() const { return m_albedo; }

private:
	glm::vec3 m_albedo{1.0f};
};