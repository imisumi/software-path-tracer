#pragma once
#include <glm/glm.hpp>
#include <vector>

struct SphereData
{
	std::vector<glm::vec3> centers;
	std::vector<float> radii;
	std::vector<uint32_t> material_indices;

	void add_sphere(glm::vec3 center, float radius, uint32_t material_index);
	void remove_sphere(uint32_t index);
	void clear();
	void reserve(size_t count);

	size_t size() const { return centers.size(); }
	bool is_empty() const { return centers.empty(); }
};