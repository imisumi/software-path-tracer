#pragma once

#include <glm/glm.hpp>

struct Ray
{
	glm::vec3 origin{0.0f};
	glm::vec3 direction{0.0f};

	Ray(const glm::vec3 &origin, const glm::vec3 &direction)
		: origin(origin), direction(direction)
	{
	}
};