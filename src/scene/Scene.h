#pragma once

#include <vector>

#include "materials/material.h"
#include "geometry/sphere_data.h"

class Scene
{
public:
	Scene();
	~Scene();

	void add_sphere(glm::vec3 center, float radius);

	const SphereData &get_sphere_data() const { return sphere_data; }
	void update_sphere(uint32_t index, glm::vec3 center, float radius, uint32_t material_index);

private:
	SphereData sphere_data;
};