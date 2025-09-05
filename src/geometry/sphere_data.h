#pragma once
#include <glm/glm.hpp>
#include <vector>

struct SphereData
{
	std::vector<float> cx;
	std::vector<float> cy;
	std::vector<float> cz;
	std::vector<float> radii;
	std::vector<uint32_t> material_indices;

	void add_sphere(glm::vec3 center, float radius, uint32_t material_index);
	void remove_sphere(uint32_t index);
	void clear();
	void reserve(size_t count);
	void update_sphere(uint32_t index, glm::vec3 center, float radius, uint32_t material_index);

	size_t size() const { return cx.size(); }
	bool is_empty() const { return cx.empty(); }
};